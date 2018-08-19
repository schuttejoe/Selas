//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SceneLib/SceneResource.h"
#include "TextureLib/TextureResource.h"
#include "Shading/SurfaceParameters.h"
#include "UtilityLib/BinarySearch.h"
#include "Assets/AssetFileUtils.h"
#include "MathLib/FloatFuncs.h"
#include "MathLib/FloatStructs.h"
#include "IoLib/BinarySerializer.h"
#include "IoLib/File.h"
#include "SystemLib/BasicTypes.h"

#include "embree3/rtcore.h"
#include "embree3/rtcore_ray.h"

#define EnableDisplacement_ 0
#define TessellationRate_ 64.0f

namespace Selas
{
    cpointer SceneResource::kDataType = "Scene";
    cpointer SceneResource::kGeometryDataType = "SceneGeometry";

    const uint64 SceneResource::kDataVersion = 1533865499ul;
    const uint32 SceneResource::kGeometryDataAlignment = 16;
    static_assert(sizeof(SceneGeometryData) % SceneResource::kGeometryDataAlignment == 0, "SceneGeometryData must be aligned");
    static_assert(SceneResource::kGeometryDataAlignment % 4 == 0, "SceneGeometryData must be aligned");

    //=============================================================================================================================
    // Embree Setup
    //=============================================================================================================================

    //=============================================================================================================================
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

    //=============================================================================================================================
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

        AssertMsg_(false, "nyi");
        uint32 geomId = 0;// (args->geometry == scene->rtcGeometries[eMeshDisplaced]) ? eMeshDisplaced : eMeshAlphaTestedDisplaced;

        for(unsigned int i = 0; i < N; i++) {
            //float3 position = float3(px[i], py[i], pz[i]);
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

    //=============================================================================================================================
    static void SetVertexAttributes(RTCGeometry geom, SceneResource* scene)
    {
        SceneMetaData* metadata = scene->data;
        SceneGeometryData* geometry = scene->geometry;

        Assert_(((uint)geometry->positions & (SceneResource::kGeometryDataAlignment - 1)) == 0);
        Assert_(((uint)geometry->normals & (SceneResource::kGeometryDataAlignment - 1)) == 0);
        Assert_(((uint)geometry->tangents & (SceneResource::kGeometryDataAlignment - 1)) == 0);
        Assert_(((uint)geometry->uvs & (SceneResource::kGeometryDataAlignment - 1)) == 0);

        rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, geometry->positions, 0, sizeof(float3), 
                                   metadata->totalVertexCount);

        rtcSetGeometryVertexAttributeCount(geom, 3);
        rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, RTC_FORMAT_FLOAT3, geometry->normals, 0, 
                                   sizeof(float3), metadata->totalVertexCount);
        rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, RTC_FORMAT_FLOAT4, geometry->tangents, 0, 
                                   sizeof(float4), metadata->totalVertexCount);
        rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 2, RTC_FORMAT_FLOAT2, geometry->uvs, 0,
                                   sizeof(float2), metadata->totalVertexCount);

        rtcSetGeometryUserData(geom, scene);
    }

    //=============================================================================================================================
    static Material* CreateDefaultMaterial()
    {
        Material* defaultMat = New_(Material);
        defaultMat->baseColor = float3(0.6f, 0.6f, 0.6f);
        defaultMat->shader = eDisneySolid;
        defaultMat->scalarAttributeValues[eIor]= 1.5f;
        
        return defaultMat;
    }

    //=============================================================================================================================
    static const Material* FindMeshMaterial(SceneResource* scene, Hash32 materialHash)
    {
        uint materialIndex = BinarySearch(scene->data->materialHashes, scene->data->materialCount, materialHash);
        if(materialIndex == (uint)-1) {
            return scene->defaultMaterial;
        }

        return &scene->data->materials[materialIndex];
    }

    //=============================================================================================================================
    static void PopulateEmbreeScene(SceneResource* scene, RTCDevice rtcDevice, RTCScene rtcScene)
    {
        SceneMetaData* sceneData = scene->data;
        SceneGeometryData* geometry = scene->geometry;

        Assert_(((uint)geometry->indices & (SceneResource::kGeometryDataAlignment - 1)) == 0);
        Assert_(((uint)geometry->faceIndexCounts & (SceneResource::kGeometryDataAlignment - 1)) == 0);

        for(uint32 scan = 0, count = sceneData->meshCount; scan < count; ++scan) {
            const MeshMetaData& meshData = sceneData->meshData[scan];
            const Material* material = FindMeshMaterial(scene, meshData.materialHash);

            bool hasDisplacement = material->flags & MaterialFlags::eDisplacementEnabled && EnableDisplacement_;
            bool hasAlphaTesting = material->flags & MaterialFlags::eAlphaTested;

            uint32 indicesPerFace = meshData.indicesPerFace;
            uint32 indexByteOffset = meshData.indexOffset * sizeof(uint32);

            RTCGeometry rtcGeometry;
            if(hasDisplacement) {
                rtcGeometry = rtcNewGeometry(rtcDevice, RTC_GEOMETRY_TYPE_SUBDIVISION);
                SetVertexAttributes(rtcGeometry, scene);
                rtcSetSharedGeometryBuffer(rtcGeometry, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT, geometry->indices,
                                           indexByteOffset, sizeof(uint32), meshData.indexCount);

                rtcSetSharedGeometryBuffer(rtcGeometry, RTC_BUFFER_TYPE_FACE, 0, RTC_FORMAT_UINT, geometry->faceIndexCounts,
                                           0, sizeof(uint32), meshData.indexCount / indicesPerFace);

                rtcSetGeometryDisplacementFunction(rtcGeometry, DisplacementFunction);
                rtcSetGeometryTessellationRate(rtcGeometry, TessellationRate_);
                rtcSetGeometrySubdivisionMode(rtcGeometry, 0, RTC_SUBDIVISION_MODE_PIN_BOUNDARY);
            }
            else {
                RTCGeometryType type = indicesPerFace == 3 ? RTC_GEOMETRY_TYPE_TRIANGLE : RTC_GEOMETRY_TYPE_QUAD;
                RTCFormat format = indicesPerFace == 3 ? RTC_FORMAT_UINT3 : RTC_FORMAT_UINT4;

                rtcGeometry = rtcNewGeometry(rtcDevice, type);
                SetVertexAttributes(rtcGeometry, scene);
                rtcSetSharedGeometryBuffer(rtcGeometry, RTC_BUFFER_TYPE_INDEX, 0, format, geometry->indices,
                                           indexByteOffset, indicesPerFace * sizeof(uint32), meshData.indexCount / indicesPerFace);
            }

            if(hasAlphaTesting) {
                rtcSetGeometryIntersectFilterFunction(rtcGeometry, IntersectionFilter);
            }

            scene->materialLookup.Add(material);
            scene->rtcGeometries.Add(rtcGeometry);

            rtcCommitGeometry(rtcGeometry);
            rtcAttachGeometryByID(rtcScene, rtcGeometry, scan);
            rtcReleaseGeometry(rtcGeometry);
        }

        rtcCommitScene(rtcScene);
    }

    //=============================================================================================================================
    // SceneResource
    //=============================================================================================================================

    //=============================================================================================================================
    SceneResource::SceneResource()
        : data(nullptr)
        , geometry(nullptr)
        , textures(nullptr)
        , rtcDevice(nullptr)
        , rtcScene(nullptr)
        , defaultMaterial(nullptr)
    {

    }

    //=============================================================================================================================
    SceneResource::~SceneResource()
    {
        Assert_(data == nullptr);
        Assert_(geometry == nullptr);
        Assert_(textures == nullptr);
        Assert_(rtcDevice == nullptr);
        Assert_(rtcScene == nullptr);
    }

    //=============================================================================================================================
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
        FixupPointerX64(fileData, data->data->materialHashes);
        FixupPointerX64(fileData, data->data->materials);
        FixupPointerX64(fileData, data->data->meshData);

        return Success_;
    }

    //=============================================================================================================================
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

        FixupPointerX64(fileData, data->geometry->indices);
        FixupPointerX64(fileData, data->geometry->faceIndexCounts);
        FixupPointerX64(fileData, data->geometry->positions);
        FixupPointerX64(fileData, data->geometry->normals);
        FixupPointerX64(fileData, data->geometry->tangents);
        FixupPointerX64(fileData, data->geometry->uvs);

        return Success_;
    }

    //=============================================================================================================================
    Error ReadSceneResource(cpointer assetname, SceneResource* data)
    {
        ReturnError_(ReadSceneMetaData(assetname, data));
        ReturnError_(ReadSceneGeometryData(assetname, data));

        return Success_;
    }

    //=============================================================================================================================
    Error InitializeSceneResource(SceneResource* scene)
    {
        uint textureCount = scene->data->textureCount;
        scene->textures = AllocArray_(TextureResource, textureCount);

        for(uint scan = 0, count = scene->data->textureCount; scan < count; ++scan) {
            ReturnError_(ReadTextureResource(scene->data->textureResourceNames[scan].Ascii(), &scene->textures[scan]));
        }

        scene->defaultMaterial = CreateDefaultMaterial();

        return Success_;
    }

    //=============================================================================================================================
    void InitializeEmbreeScene(SceneResource* scene)
    {
        RTCDevice rtcDevice = rtcNewDevice(nullptr/*"verbose=3"*/);
        RTCScene rtcScene = rtcNewScene(rtcDevice);

        scene->rtcDevice = (void*)rtcDevice;
        scene->rtcScene = (void*)rtcScene;

        PopulateEmbreeScene(scene, rtcDevice, rtcScene);
    }

    //=============================================================================================================================
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

        SafeDelete_(scene->defaultMaterial);
        SafeFree_(scene->textures);
        SafeFreeAligned_(scene->data);
        SafeFreeAligned_(scene->geometry);
    }
}
