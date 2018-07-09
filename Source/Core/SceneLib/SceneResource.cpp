//==============================================================================
// Joe Schutte
//==============================================================================

#include "SceneLib/SceneResource.h"
#include "TextureLib/TextureResource.h"
#include "Shading/SurfaceParameters.h"
#include "Assets/AssetFileUtils.h"
#include "MathLib/FloatFuncs.h"
#include "MathLib/FloatStructs.h"
#include "IoLib/BinarySerializer.h"
#include "IoLib/File.h"
#include "SystemLib/BasicTypes.h"

#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>

#define EnableDisplacement_ 0
#define TessellationRate_ 64.0f

namespace Selas
{
    cpointer SceneResource::kDataType = "Scene";
    cpointer SceneResource::kGeometryDataType = "SceneGeometry";

    const uint64 SceneResource::kDataVersion = 1529795915ul;
    const uint32 SceneResource::kSceneDataAlignment = 16;
    static_assert(sizeof(SceneGeometryData) % SceneResource::kSceneDataAlignment == 0, "SceneGeometryData must be aligned");

    //==============================================================================
    // Embree Setup
    //==============================================================================

    //==============================================================================
    static void IntersectionFilter(const RTCFilterFunctionNArguments* args)
    {
        int* valid = args->valid;
        SceneResource* scene = (SceneResource*)args->geometryUserPtr;

        for(uint32 scan = 0; scan < args->N; ++scan) {
            if(valid[scan] != -1) {
                continue;
            }

            RTCHit hit = rtcGetHitFromHitN(args->hit, args->N, scan);
            valid[scan] = CalculatePassesAlphaTest(scene, hit.geomID, hit.primID, { hit.u, hit.v });
        }
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
    static void PopulateEmbreeScene(SceneResource* scene, RTCDevice rtcDevice, RTCScene rtcScene)
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
    // SceneResource
    //==============================================================================

    //==============================================================================
    SceneResource::SceneResource()
        : data(nullptr)
        , geometry(nullptr)
        , textures(nullptr)
        , rtcDevice(nullptr)
        , rtcScene(nullptr)
    {
        Memory::Zero(&rtcGeometries[0], sizeof(rtcGeometries));
    }

    //==============================================================================
    SceneResource::~SceneResource()
    {
        Assert_(data == nullptr);
        Assert_(geometry == nullptr);
        Assert_(textures == nullptr);
        Assert_(rtcDevice == nullptr);
        Assert_(rtcScene == nullptr);
    }

    //==============================================================================
    static Error ReadSceneMetaData(cpointer assetname, SceneResource* data)
    {
        FilePathString filepath;
        AssetFileUtils::AssetFilePath(SceneResource::kDataType, SceneResource::kDataVersion, assetname, filepath);

        void* fileData = nullptr;
        uint32 fileSize = 0;
        ReturnError_(File::ReadWholeFile(filepath.Ascii(), &fileData, &fileSize));

        BinaryReader reader;
        SerializerStart(&reader, fileData, fileSize);

        SerializerAttach(&reader, reinterpret_cast<void**>(&data->data), fileSize);
        SerializerEnd(&reader);

        FixupPointerX64(fileData, data->data->textureResourceNames);
        FixupPointerX64(fileData, data->data->materials);

        return Success_;
    }

    //==============================================================================
    static Error ReadSceneGeometryData(cpointer assetname, SceneResource* data)
    {
        FilePathString filepath;
        AssetFileUtils::AssetFilePath(SceneResource::kGeometryDataType, SceneResource::kDataVersion, assetname, filepath);

        void* fileData = nullptr;
        uint32 fileSize = 0;
        ReturnError_(File::ReadWholeFile(filepath.Ascii(), &fileData, &fileSize));

        BinaryReader reader;
        SerializerStart(&reader, fileData, fileSize);
        SerializerAttach(&reader, reinterpret_cast<void**>(&data->geometry), fileSize);
        SerializerEnd(&reader);

        for(uint scan = 0; scan < eMeshIndexTypeCount; ++scan) {
            FixupPointerX64(fileData, data->geometry->indices[scan]);
        }
        FixupPointerX64(fileData, data->geometry->faceIndexCounts);
        FixupPointerX64(fileData, data->geometry->positions);
        FixupPointerX64(fileData, data->geometry->normals);
        FixupPointerX64(fileData, data->geometry->tangents);
        FixupPointerX64(fileData, data->geometry->uvs);
        FixupPointerX64(fileData, data->geometry->materialIndices);

        return Success_;
    }

    //==============================================================================
    Error ReadSceneResource(cpointer assetname, SceneResource* data)
    {
        ReturnError_(ReadSceneMetaData(assetname, data));
        ReturnError_(ReadSceneGeometryData(assetname, data));

        return Success_;
    }

    //==============================================================================
    Error InitializeSceneResource(SceneResource* scene)
    {
        // -- JSTODO - Should be fetching other resource data from some asset mgr here rather than directly loading it

        uint textureCount = scene->data->textureCount;
        scene->textures = AllocArray_(TextureResource, textureCount);

        for(uint scan = 0, count = scene->data->textureCount; scan < count; ++scan) {
            ReturnError_(ReadTextureResource(scene->data->textureResourceNames[scan].Ascii(), &scene->textures[scan]));
        }

        return Success_;
    }

    //==============================================================================
    void InitializeEmbreeScene(SceneResource* scene)
    {
        RTCDevice rtcDevice = rtcNewDevice(nullptr/*"verbose=3"*/);
        RTCScene rtcScene = rtcNewScene(rtcDevice);

        scene->rtcDevice = (void*)rtcDevice;
        scene->rtcScene = (void*)rtcScene;

        PopulateEmbreeScene(scene, rtcDevice, rtcScene);
    }

    //==============================================================================
    void ShutdownSceneResource(SceneResource* scene)
    {
        if(scene->rtcScene)
            rtcReleaseScene((RTCScene)scene->rtcScene);
        if(scene->rtcDevice)
            rtcReleaseDevice((RTCDevice)scene->rtcDevice);

        scene->rtcScene = nullptr;
        scene->rtcDevice = nullptr;

        for(uint scan = 0, count = scene->data->textureCount; scan < count; ++scan) {
            ShutdownTextureResource(&scene->textures[scan]);
        }

        SafeFree_(scene->textures);
        SafeFreeAligned_(scene->data);
        SafeFreeAligned_(scene->geometry);
    }
}