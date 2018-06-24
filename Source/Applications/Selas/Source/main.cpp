
//==============================================================================
// Joe Schutte
//==============================================================================

#include "PathTracer.h"
#include "VCM.h"
#include "Shading/IntegratorContexts.h"

#include "Shading/SurfaceParameters.h"
#include "SceneLib/SceneResource.h"
#include "SceneLib/ImageBasedLightResource.h"
#include "TextureLib/Framebuffer.h"
#include "TextureLib/StbImage.h"
#include "TextureLib/TextureFiltering.h"
#include "TextureLib/TextureResource.h"
#include "IoLib/Environment.h"
#include "StringLib/FixedString.h"
#include "SystemLib/Error.h"
#include "SystemLib/MemoryAllocation.h"
#include "SystemLib/BasicTypes.h"
#include "SystemLib/Memory.h"
#include "SystemLib/SystemTime.h"
#include "SystemLib/Logging.h"

#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>

#include "xmmintrin.h"
#include "pmmintrin.h"
#include <stdio.h>

using namespace Selas;

#define EnableDisplacement_ 0
#define TessellationRate_ 64.0f

//==============================================================================
static void IntersectionFilter(const RTCFilterFunctionNArguments* args)
{
    Assert_(args->N == 1);
    int* valid = args->valid;
    if(valid[0] != -1) {
        return;
    }

    SceneResource* scene = (SceneResource*)args->geometryUserPtr;

    RTCHit hit = rtcGetHitFromHitN(args->hit, args->N, 0);
    valid[0] = CalculatePassesAlphaTest(scene, hit.geomID, hit.primID, { hit.u, hit.v });
}

#if EnableDisplacement_
//==============================================================================
static void DisplacementFunction(const RTCDisplacementFunctionNArguments* args)
{
    const float* nx = args->Ng_x;
    const float* ny = args->Ng_y;
    const float* nz = args->Ng_z;

    const float* us = args->u;
    const float* vs = args->v;

    float* px = args->P_x;
    float* py = args->P_y;
    float* pz = args->P_z;

    unsigned int N = args->N;

    SceneResource* scene = (SceneResource*)args->geometryUserPtr;

    uint32 geomId = (args->geometry == scene->rtcGeometries[eMeshDisplaced]) ? eMeshDisplaced : eMeshAlphaTestedDisplaced;

    for(unsigned int i = 0; i < N; i++) {
        float3 position = float3(px[i], py[i], pz[i]);
        float3 normal = float3(nx[i], ny[i], nz[i]);
        float2 barys = float2(us[i], vs[i]);

        Align_(16) float2 uvs;
        rtcInterpolate0(args->geometry, args->primID, barys.x, barys.y, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 2, &uvs.x, 2);

        float displacement = CalculateDisplacement(scene, geomId, args->primID, uvs);

        #if CheckForNaNs_
            Assert_(!Math::IsNaN(normal.x));
            Assert_(!Math::IsNaN(normal.y));
            Assert_(!Math::IsNaN(normal.z));
            Assert_(!Math::IsNaN(displacement));
        #endif

        float3 deltaPosition = displacement * normal;

        px[i] += deltaPosition.x;
        py[i] += deltaPosition.y;
        pz[i] += deltaPosition.z;
    }
}
#endif

//==============================================================================
static void SetVertexAttributes(RTCGeometry geom, SceneResource* scene)
{
    SceneMetaData* metadata = scene->data;
    SceneGeometryData* geometry = scene->geometry;

    Assert_(((uint)geometry->positions & (SceneResource::kSceneDataAlignment - 1)) == 0);
    

    Assert_(((uint)geometry->normals & (SceneResource::kSceneDataAlignment - 1)) == 0);
    Assert_(((uint)geometry->tangents & (SceneResource::kSceneDataAlignment - 1)) == 0);
    Assert_(((uint)geometry->uvs & (SceneResource::kSceneDataAlignment - 1)) == 0);

    rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, geometry->positions, 0, sizeof(float3), metadata->totalVertexCount);

    rtcSetGeometryVertexAttributeCount(geom, 3);
    rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, RTC_FORMAT_FLOAT3, geometry->normals, 0, sizeof(float3), metadata->totalVertexCount);
    rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, RTC_FORMAT_FLOAT4, geometry->tangents, 0, sizeof(float4), metadata->totalVertexCount);
    rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 2, RTC_FORMAT_FLOAT2, geometry->uvs, 0, sizeof(float2), metadata->totalVertexCount);

    rtcSetGeometryUserData(geom, scene);
}

//==============================================================================
static void PopulateEmbreeScene(SceneResource* scene, RTCDevice& rtcDevice, RTCScene& rtcScene)
{
    SceneMetaData* metadata = scene->data;
    SceneGeometryData* geometry = scene->geometry;

    for(uint scan = 0; scan < eMeshIndexTypeCount; ++scan) {
        Assert_(((uint)geometry->indices[scan] & (SceneResource::kSceneDataAlignment - 1)) == 0);
    }
    Assert_(((uint)geometry->faceIndexCounts & (SceneResource::kSceneDataAlignment - 1)) == 0);

    if(metadata->indexCounts[eMeshStandard] > 0) {
        RTCGeometry solidMeshHandle = rtcNewGeometry(rtcDevice, RTC_GEOMETRY_TYPE_TRIANGLE);
        SetVertexAttributes(solidMeshHandle, scene);
        rtcSetSharedGeometryBuffer(solidMeshHandle, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, geometry->indices[eMeshStandard], 0, 3 * sizeof(uint32), metadata->indexCounts[eMeshStandard] / 3);
        rtcCommitGeometry(solidMeshHandle);
        rtcAttachGeometryByID(rtcScene, solidMeshHandle, eMeshStandard);
        rtcReleaseGeometry(solidMeshHandle);
        scene->rtcGeometries[eMeshStandard] = solidMeshHandle;
    }

    if(metadata->indexCounts[eMeshAlphaTested] > 0) {
        RTCGeometry atMeshHandle = rtcNewGeometry(rtcDevice, RTC_GEOMETRY_TYPE_TRIANGLE);
        SetVertexAttributes(atMeshHandle, scene);
        rtcSetSharedGeometryBuffer(atMeshHandle, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, geometry->indices[eMeshAlphaTested], 0, 3 * sizeof(uint32), metadata->indexCounts[eMeshAlphaTested] / 3);
        rtcSetGeometryIntersectFilterFunction(atMeshHandle, IntersectionFilter);
        rtcCommitGeometry(atMeshHandle);
        rtcAttachGeometryByID(rtcScene, atMeshHandle, eMeshAlphaTested);
        rtcReleaseGeometry(atMeshHandle);
        scene->rtcGeometries[eMeshAlphaTested] = atMeshHandle;
    }

    if(metadata->indexCounts[eMeshDisplaced] > 0) {
        #if EnableDisplacement_
            RTCGeometry dispMeshHandle = rtcNewGeometry(rtcDevice, RTC_GEOMETRY_TYPE_SUBDIVISION);
        
            SetVertexAttributes(dispMeshHandle, scene);
            rtcSetSharedGeometryBuffer(dispMeshHandle, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT, geometry->indices[eMeshDisplaced], 0, sizeof(uint32), metadata->indexCounts[eMeshDisplaced]);
            rtcSetSharedGeometryBuffer(dispMeshHandle, RTC_BUFFER_TYPE_FACE, 0, RTC_FORMAT_UINT, geometry->faceIndexCounts, 0, sizeof(uint32), metadata->indexCounts[eMeshDisplaced] / 3);

            rtcSetGeometryDisplacementFunction(dispMeshHandle, DisplacementFunction);
            rtcSetGeometryTessellationRate(dispMeshHandle, TessellationRate_);
            rtcSetGeometrySubdivisionMode(dispMeshHandle, 0, RTC_SUBDIVISION_MODE_PIN_BOUNDARY);

            rtcCommitGeometry(dispMeshHandle);
            rtcAttachGeometryByID(rtcScene, dispMeshHandle, eMeshDisplaced);
            rtcReleaseGeometry(dispMeshHandle);
            scene->rtcGeometries[eMeshDisplaced] = dispMeshHandle;
        #else
            RTCGeometry dispMeshHandle = rtcNewGeometry(rtcDevice, RTC_GEOMETRY_TYPE_TRIANGLE);
            SetVertexAttributes(dispMeshHandle, scene);
            rtcSetSharedGeometryBuffer(dispMeshHandle, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, geometry->indices[eMeshDisplaced], 0, 3 * sizeof(uint32), metadata->indexCounts[eMeshDisplaced] / 3);
            rtcCommitGeometry(dispMeshHandle);
            rtcAttachGeometryByID(rtcScene, dispMeshHandle, eMeshDisplaced);
            rtcReleaseGeometry(dispMeshHandle);
            scene->rtcGeometries[eMeshDisplaced] = dispMeshHandle;
        #endif
    }

    if(metadata->indexCounts[eMeshAlphaTestedDisplaced] > 0) {
        #if EnableDisplacement_
            RTCGeometry dispAtMeshHandle = rtcNewGeometry(rtcDevice, RTC_GEOMETRY_TYPE_SUBDIVISION);
            SetVertexAttributes(dispAtMeshHandle, scene);
            rtcSetSharedGeometryBuffer(dispAtMeshHandle, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT, geometry->indices[eMeshAlphaTestedDisplaced], 0, sizeof(uint32), metadata->indexCounts[eMeshAlphaTestedDisplaced]);
            rtcSetSharedGeometryBuffer(dispAtMeshHandle, RTC_BUFFER_TYPE_FACE, 0, RTC_FORMAT_UINT, geometry->faceIndexCounts, 0, sizeof(uint32), metadata->indexCounts[eMeshAlphaTestedDisplaced] / 3);

            rtcSetGeometryDisplacementFunction(dispAtMeshHandle, DisplacementFunction);
            rtcSetGeometryIntersectFilterFunction(dispAtMeshHandle, IntersectionFilter);
            rtcSetGeometryTessellationRate(dispAtMeshHandle, TessellationRate_);
            rtcSetGeometrySubdivisionMode(dispAtMeshHandle, 0, RTC_SUBDIVISION_MODE_PIN_BOUNDARY);
            rtcCommitGeometry(dispAtMeshHandle);
            rtcAttachGeometryByID(rtcScene, dispAtMeshHandle, eMeshAlphaTestedDisplaced);
            rtcReleaseGeometry(dispAtMeshHandle);
            scene->rtcGeometries[eMeshAlphaTestedDisplaced] = dispAtMeshHandle;
        #else
            RTCGeometry dispAtMeshHandle = rtcNewGeometry(rtcDevice, RTC_GEOMETRY_TYPE_TRIANGLE);
            SetVertexAttributes(dispAtMeshHandle, scene);
            rtcSetSharedGeometryBuffer(dispAtMeshHandle, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, geometry->indices[eMeshAlphaTestedDisplaced], 0, 3 * sizeof(uint32), metadata->indexCounts[eMeshAlphaTestedDisplaced] / 3);
            rtcCommitGeometry(dispAtMeshHandle);
            rtcAttachGeometryByID(rtcScene, dispAtMeshHandle, eMeshAlphaTestedDisplaced);
            rtcReleaseGeometry(dispAtMeshHandle);
            scene->rtcGeometries[eMeshAlphaTestedDisplaced] = dispAtMeshHandle;
        #endif
    }

    rtcCommitScene(rtcScene);
}

//==============================================================================
int main(int argc, char *argv[])
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    Environment_Initialize(ProjectRootName_, argv[0]);

    int retvalue = 0;

    TextureFiltering::InitializeEWAFilterWeights();

    RTCDevice rtcDevice = rtcNewDevice(nullptr/*"verbose=3"*/);
    RTCScene rtcScene = rtcNewScene(rtcDevice);

    auto timer = SystemTime::Now();

    SceneResource sceneResource;
    ExitMainOnError_(ReadSceneResource("Scenes~SanMiguel~SanMiguel.fbx", &sceneResource));
    //ExitMainOnError_(ReadSceneResource("Meshes~plane_with_sphere.fbx", &sceneResource));
    //ExitMainOnError_(ReadSceneResource("Meshes~DisplacementTest.fbx", &sceneResource));
    ExitMainOnError_(InitializeSceneResource(&sceneResource));

    ImageBasedLightResource iblResouce;
    ExitMainOnError_(ReadImageBasedLightResource("HDR~noon_grass_4k_upper.hdr", &iblResouce));

    float elapsedMs = SystemTime::ElapsedMillisecondsF(timer);
    WriteDebugInfo_("Scene load time %fms", elapsedMs);

    timer = SystemTime::Now();
    
    PopulateEmbreeScene(&sceneResource, rtcDevice, rtcScene);
    
    elapsedMs = SystemTime::ElapsedMillisecondsF(timer);
    WriteDebugInfo_("Scene build time %fms", elapsedMs);

    //sceneResource.data->camera.fov = 0.7f;
    //uint width = 256;
    //uint height = 256;
    Selas::uint width = 1920;
    Selas::uint height = 1080;

    Framebuffer frame;
    FrameBuffer_Initialize(&frame, (uint32)width, (uint32)height);

    float sceneBoundingRadius = sceneResource.data->boundingSphere.w;

    SceneContext context;
    context.rtcScene = rtcScene;
    context.scene = &sceneResource;
    context.ibl = iblResouce.data;

    timer = SystemTime::Now();

    //PathTracer::GenerateImage(context, width, height, imageData);
    VCM::GenerateImage(context, &frame);

    elapsedMs = SystemTime::ElapsedMillisecondsF(timer);
    WriteDebugInfo_("Scene render time %fms", elapsedMs);

    FrameBuffer_Save(&frame, "temp");
    FrameBuffer_Shutdown(&frame);

    // -- delete the scene
    ShutdownSceneResource(&sceneResource);
    SafeFreeAligned_(iblResouce.data);

    rtcReleaseScene(rtcScene);
    rtcReleaseDevice(rtcDevice);

    return retvalue;
}