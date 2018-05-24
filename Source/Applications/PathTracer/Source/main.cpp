
//==============================================================================
// Joe Schutte
//==============================================================================

#include "PathTracer.h"
#include "VCM.h"
#include <Shading/IntegratorContexts.h>

#include <SceneLib/SceneResource.h>
#include <SceneLib/ImageBasedLightResource.h>
#include <TextureLib/StbImage.h>
#include <TextureLib/TextureFiltering.h>
#include <TextureLib/TextureResource.h>
#include <StringLib/FixedString.h>
#include <SystemLib/MemoryAllocation.h>
#include <SystemLib/BasicTypes.h>
#include <SystemLib/Memory.h>
#include <SystemLib/SystemTime.h>

#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>

#include "xmmintrin.h"
#include "pmmintrin.h"
#include <stdio.h>
#include <windows.h>

using namespace Shooty;

//==============================================================================
static uint32 PopulateEmbreeScene(SceneResourceData* sceneData, RTCDevice& rtcDevice, RTCScene& rtcScene) {

    uint32 vertexCount = sceneData->totalVertexCount;
    uint32 indexCount = sceneData->totalIndexCount;
    uint32 triangleCount = indexCount / 3;

    RTCGeometry rtcMeshHandle = rtcNewGeometry(rtcDevice, RTC_GEOMETRY_TYPE_TRIANGLE);
   
    rtcSetSharedGeometryBuffer(rtcMeshHandle, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sceneData->positions, 0, sizeof(float3), sceneData->totalVertexCount);
    rtcSetSharedGeometryBuffer(rtcMeshHandle, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sceneData->indices, 0, 3 * sizeof(uint32), sceneData->totalIndexCount / 3);
    rtcCommitGeometry(rtcMeshHandle);

    unsigned int geomID = rtcAttachGeometry(rtcScene, rtcMeshHandle);
    rtcReleaseGeometry(rtcMeshHandle);

    rtcCommitScene(rtcScene);

    return geomID;
}

//==============================================================================
int main()
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    int retvalue = 0;

    TextureFiltering::InitializeEWAFilterWeights();

    RTCDevice rtcDevice = rtcNewDevice(nullptr/*"verbose=3"*/);
    RTCScene rtcScene = rtcNewScene(rtcDevice);
    uint32 meshHandle = -1;

    SceneResource sceneResource;
    if(ReadSceneResource("D:\\Shooty\\ShootyEngine\\_Assets\\Scenes\\plane_with_sphere", &sceneResource) == false) {
        retvalue = -1;
        goto cleanup;
    }
    if(InitializeSceneResource(&sceneResource) == false) {
        retvalue = -1;
        goto cleanup;
    }

    ImageBasedLightResource iblResouce;
    if(ReadImageBasedLightResource("D:\\Shooty\\ShootyEngine\\_Assets\\IBLs\\simons_town_rocks_4k_upper.bin", &iblResouce) == false) {
        retvalue = -1;
        goto cleanup;
    }

    int64 timer;

    SystemTime::GetCycleCounter(&timer);
    meshHandle = PopulateEmbreeScene(sceneResource.data, rtcDevice, rtcScene);
    float buildms = SystemTime::ElapsedMs(timer);

    //sceneResource.data->camera.fov = 0.7f;
    //uint width = 256;
    //uint height = 256;
    uint width = 1280;
    uint height = 720;

    float3* imageData = AllocArray_(float3, width * height);
    Memory::Zero(imageData, sizeof(float3) * width * height);

    float sceneBoundingRadius = sceneResource.data->boundingSphere.w;

    SceneContext context;
    context.rtcScene = rtcScene;
    context.scene = &sceneResource;
    context.ibl = iblResouce.data;

    SystemTime::GetCycleCounter(&timer);
    VCM::GenerateImage(context, width, height, imageData);
    float renderms = SystemTime::ElapsedMs(timer);

    StbImageWrite("D:\\temp\\test.hdr", width, height, 3, HDR, imageData);
    Free_(imageData);

    FixedString64 buildlog;
    sprintf_s(buildlog.Ascii(), buildlog.Capcaity(), "Scene build time %fms\n", buildms);

    FixedString64 renderlog;
    sprintf_s(renderlog.Ascii(), renderlog.Capcaity(), "Scene render time %fms\n", renderms);

    OutputDebugStringA(buildlog.Ascii());
    OutputDebugStringA(renderlog.Ascii());

cleanup:
    // -- delete the scene
    ShutdownSceneResource(&sceneResource);
    SafeFreeAligned_(iblResouce.data);

    rtcReleaseScene(rtcScene);
    rtcReleaseDevice(rtcDevice);

    return retvalue;
}