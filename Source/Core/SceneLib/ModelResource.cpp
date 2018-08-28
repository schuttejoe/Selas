//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SceneLib/ModelResource.h"
#include "TextureLib/TextureResource.h"
#include "Shading/SurfaceParameters.h"
#include "UtilityLib/BinarySearch.h"
#include "Assets/AssetFileUtils.h"
#include "MathLib/FloatFuncs.h"
#include "MathLib/FloatStructs.h"
#include "IoLib/File.h"
#include "IoLib/BinaryStreamSerializer.h"
#include "SystemLib/BasicTypes.h"

#include "embree3/rtcore.h"
#include "embree3/rtcore_ray.h"

#define EnableDisplacement_ 0
#define TessellationRate_ 64.0f

namespace Selas
{
    cpointer ModelResource::kDataType = "ModelResource";
    cpointer ModelResource::kGeometryDataType = "ModelGeometryResource";

    const uint64 ModelResource::kDataVersion = 1535305887ul;
    const uint32 ModelResource::kGeometryDataAlignment = 16;
    static_assert(sizeof(ModelGeometryData) % ModelResource::kGeometryDataAlignment == 0, "SceneGeometryData must be aligned");
    static_assert(ModelResource::kGeometryDataAlignment % 4 == 0, "SceneGeometryData must be aligned");

    //=============================================================================================================================
    // Serialization
    //=============================================================================================================================

    //=============================================================================================================================
    void Serialize(CSerializer* serializer, ModelResourceData& data)
    {
        Serialize(serializer, data.aaBox);
        Serialize(serializer, data.boundingSphere);
        Serialize(serializer, data.cameraCount);
        Serialize(serializer, data.meshCount);
        Serialize(serializer, data.totalVertexCount);
        Serialize(serializer, data.indexCount);
        Serialize(serializer, data.textureCount);
        Serialize(serializer, data.materialCount);

        serializer->SerializePtr((void*&)data.cameras, data.cameraCount * sizeof(CameraSettings), 0);
        serializer->SerializePtr((void*&)data.textureResourceNames, data.textureCount * sizeof(FilePathString), 0);
        serializer->SerializePtr((void*&)data.materials, data.materialCount * sizeof(Material), 0);
        serializer->SerializePtr((void*&)data.materialHashes, data.materialCount * sizeof(Hash32), 0);
        serializer->SerializePtr((void*&)data.meshData, data.meshCount * sizeof(MeshMetaData), 0);
    }

    //=============================================================================================================================
    void Serialize(CSerializer* serializer, ModelGeometryData& data)
    {
        Serialize(serializer, data.indexSize);
        Serialize(serializer, data.faceIndexSize);
        Serialize(serializer, data.positionSize);
        Serialize(serializer, data.normalsSize);
        Serialize(serializer, data.tangentsSize);
        Serialize(serializer, data.uvsSize);
        
        serializer->SerializePtr((void*&)data.indices, data.indexSize, ModelResource::kGeometryDataAlignment);
        serializer->SerializePtr((void*&)data.faceIndexCounts, data.faceIndexSize, ModelResource::kGeometryDataAlignment);
        serializer->SerializePtr((void*&)data.positions, data.positionSize, ModelResource::kGeometryDataAlignment);
        serializer->SerializePtr((void*&)data.normals, data.normalsSize, ModelResource::kGeometryDataAlignment);
        serializer->SerializePtr((void*&)data.tangents, data.tangentsSize, ModelResource::kGeometryDataAlignment);
        serializer->SerializePtr((void*&)data.uvs, data.uvsSize, ModelResource::kGeometryDataAlignment);
    }

    //=============================================================================================================================
    // Embree Setup
    //=============================================================================================================================

    //=============================================================================================================================
    static void IntersectionFilter(const RTCFilterFunctionNArguments* args)
    {
        int* valid = args->valid;
        ModelResource* model = (ModelResource*)args->geometryUserPtr;

        for(uint32 scan = 0; scan < args->N; ++scan) {
            if(valid[scan] != -1) {
                continue;
            }

            RTCHit hit = rtcGetHitFromHitN(args->hit, args->N, scan);
            valid[scan] = CalculatePassesAlphaTest(model, hit.geomID, hit.primID, { hit.u, hit.v });
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

        ModelResource* model = (ModelResource*)args->geometryUserPtr;

        AssertMsg_(false, "nyi");
        uint32 geomId = 0;// (args->geometry == model->rtcGeometries[eMeshDisplaced]) ? eMeshDisplaced : eMeshAlphaTestedDisplaced;

        for(unsigned int i = 0; i < N; i++) {
            //float3 position = float3(px[i], py[i], pz[i]);
            float3 normal = float3(nx[i], ny[i], nz[i]);
            float2 barys = float2(us[i], vs[i]);

            Align_(16) float2 uvs;
            rtcInterpolate0(args->geometry, args->primID, barys.x, barys.y, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 2, &uvs.x, 2);

            float displacement = CalculateDisplacement(model, geomId, args->primID, uvs);

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
    static void SetVertexAttributes(RTCGeometry geom, ModelResource* model)
    {
        ModelResourceData* metadata = model->data;
        ModelGeometryData* geometry = model->geometry;

        Assert_(((uint)geometry->positions & (ModelResource::kGeometryDataAlignment - 1)) == 0);
        Assert_(((uint)geometry->normals & (ModelResource::kGeometryDataAlignment - 1)) == 0);
        Assert_(((uint)geometry->tangents & (ModelResource::kGeometryDataAlignment - 1)) == 0);
        Assert_(((uint)geometry->uvs & (ModelResource::kGeometryDataAlignment - 1)) == 0);

        rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, geometry->positions, 0, sizeof(float3), 
                                   metadata->totalVertexCount);

        rtcSetGeometryVertexAttributeCount(geom, 3);
        rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, RTC_FORMAT_FLOAT3, geometry->normals, 0, 
                                   sizeof(float3), metadata->totalVertexCount);
        rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, RTC_FORMAT_FLOAT4, geometry->tangents, 0, 
                                   sizeof(float4), metadata->totalVertexCount);
        rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 2, RTC_FORMAT_FLOAT2, geometry->uvs, 0,
                                   sizeof(float2), metadata->totalVertexCount);

        rtcSetGeometryUserData(geom, model);
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
    static const Material* FindMeshMaterial(ModelResource* model, Hash32 materialHash)
    {
        if(model->data->materialCount == 0) {
            return model->defaultMaterial;
        }

        uint materialIndex = BinarySearch(model->data->materialHashes, model->data->materialCount, materialHash);
        if(materialIndex == (uint)-1) {
            return model->defaultMaterial;
        }

        return &model->data->materials[materialIndex];
    }

    //=============================================================================================================================
    static void PopulateEmbreeScene(ModelResource* model, RTCDevice rtcDevice, RTCScene rtcScene)
    {
        ModelResourceData* modelData = model->data;
        ModelGeometryData* geometry = model->geometry;

        Assert_(((uint)geometry->indices & (ModelResource::kGeometryDataAlignment - 1)) == 0);
        Assert_(((uint)geometry->faceIndexCounts & (ModelResource::kGeometryDataAlignment - 1)) == 0);

        for(uint32 scan = 0, count = modelData->meshCount; scan < count; ++scan) {
            const MeshMetaData& meshData = modelData->meshData[scan];
            const Material* material = FindMeshMaterial(model, meshData.materialHash);

            bool hasDisplacement = material->flags & MaterialFlags::eDisplacementEnabled && EnableDisplacement_;
            bool hasAlphaTesting = material->flags & MaterialFlags::eAlphaTested;

            uint32 indicesPerFace = meshData.indicesPerFace;
            uint32 indexByteOffset = meshData.indexOffset * sizeof(uint32);

            RTCGeometry rtcGeometry;
            if(hasDisplacement) {
                rtcGeometry = rtcNewGeometry(rtcDevice, RTC_GEOMETRY_TYPE_SUBDIVISION);
                SetVertexAttributes(rtcGeometry, model);
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
                SetVertexAttributes(rtcGeometry, model);
                rtcSetSharedGeometryBuffer(rtcGeometry, RTC_BUFFER_TYPE_INDEX, 0, format, geometry->indices,
                                           indexByteOffset, indicesPerFace * sizeof(uint32), meshData.indexCount / indicesPerFace);
            }

            if(hasAlphaTesting) {
                rtcSetGeometryIntersectFilterFunction(rtcGeometry, IntersectionFilter);
            }

            model->materialLookup.Add(material);
            model->rtcGeometries.Add(rtcGeometry);

            rtcCommitGeometry(rtcGeometry);
            rtcAttachGeometryByID(rtcScene, rtcGeometry, scan);
            rtcReleaseGeometry(rtcGeometry);
        }

        rtcCommitScene(rtcScene);
    }

    //=============================================================================================================================
    // ModelResource
    //=============================================================================================================================

    //=============================================================================================================================
    ModelResource::ModelResource()
        : data(nullptr)
        , geometry(nullptr)
        , textures(nullptr)
        , rtcScene(nullptr)
        , defaultMaterial(nullptr)
    {

    }

    //=============================================================================================================================
    ModelResource::~ModelResource()
    {
        Assert_(data == nullptr);
        Assert_(geometry == nullptr);
        Assert_(textures == nullptr);
        Assert_(rtcScene == nullptr);
    }

    //=============================================================================================================================
    static Error ReadModelResourceData(cpointer assetname, ModelResource* data)
    {
        FilePathString filepath;
        AssetFileUtils::AssetFilePath(ModelResource::kDataType, ModelResource::kDataVersion, assetname, filepath);

        void* fileData = nullptr;
        uint64 fileSize = 0;
        ReturnError_(File::ReadWholeFile(filepath.Ascii(), &fileData, &fileSize));

        AttachToBinary(data->data, (uint8*)fileData, fileSize);

        return Success_;
    }

    //=============================================================================================================================
    static Error ReadModelGeometryData(cpointer assetname, ModelResource* data)
    {
        FilePathString filepath;
        AssetFileUtils::AssetFilePath(ModelResource::kGeometryDataType, ModelResource::kDataVersion, assetname, filepath);

        void* fileData = nullptr;
        uint64 fileSize = 0;
        ReturnError_(File::ReadWholeFile(filepath.Ascii(), &fileData, &fileSize));

        AttachToBinary(data->geometry, (uint8*)fileData, fileSize);

        return Success_;
    }

    //=============================================================================================================================
    Error ReadModelResource(cpointer assetname, ModelResource* data)
    {
        ReturnError_(ReadModelResourceData(assetname, data));
        ReturnError_(ReadModelGeometryData(assetname, data));

        return Success_;
    }

    //=============================================================================================================================
    Error InitializeModelResource(ModelResource* model)
    {
        uint textureCount = model->data->textureCount;
        model->textures = AllocArray_(TextureResource, textureCount);

        for(uint scan = 0, count = model->data->textureCount; scan < count; ++scan) {
            ReturnError_(ReadTextureResource(model->data->textureResourceNames[scan].Ascii(), &model->textures[scan]));
        }

        model->defaultMaterial = CreateDefaultMaterial();

        return Success_;
    }

    //=============================================================================================================================
    void InitializeEmbreeScene(ModelResource* model, RTCDevice rtcDevice)
    {
        RTCScene rtcScene = rtcNewScene(rtcDevice);

        model->rtcScene = rtcScene;

        PopulateEmbreeScene(model, rtcDevice, rtcScene);
    }

    //=============================================================================================================================
    void ShutdownModelResource(ModelResource* model)
    {
        if(model->rtcScene)
            rtcReleaseScene(model->rtcScene);
            
        model->rtcScene = nullptr;

        for(uint scan = 0, count = model->data->textureCount; scan < count; ++scan) {
            ShutdownTextureResource(&model->textures[scan]);
        }

        SafeDelete_(model->defaultMaterial);
        SafeFree_(model->textures);
        SafeFreeAligned_(model->data);
        SafeFreeAligned_(model->geometry);
    }
}
