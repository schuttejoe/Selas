#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "StringLib/FixedString.h"
#include "GeometryLib/AxisAlignedBox.h"
#include "GeometryLib/Camera.h"
#include "UtilityLib/MurmurHash.h"
#include "MathLib/FloatStructs.h"
#include "ContainersLib/CArray.h"
#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    #pragma warning(default : 4820)

    struct TextureResource;
    struct HitParameters;

    enum eMaterialShader
    {
        eDisneyThin,
        eDisneySolid,

        eShaderCount
    };

    enum MaterialFlags
    {
        eTransparent              = 1 << 0,
        eAlphaTested              = 1 << 1,
        eDisplacementEnabled      = 1 << 2,
        eInvertDisplacement       = 1 << 3
    };

    enum ScalarMaterialProperties
    {
        // -- Disney BSDF parameters
        eMetallic,
        eSpecularTint,
        eRoughness,
        eAnisotropic,
        eSheen,
        eSheenTint,
        eClearcoat,
        eClearcoatGloss,
        eSpecTrans,
        eDiffuseTrans,
        eFlatness,
        eIor,
        eScatterDistance,

        // -- other properties
        eDisplacement,

        eMaterialPropertyCount
    };

    struct Material
    {
        Material()
            : baseColorTextureIndex(InvalidIndex32)
            , normalTextureIndex(InvalidIndex32)
            , flags(0)
            , baseColor(float3::Zero_)
            , transmittanceColor(float3::Zero_)
            , shader(eDisneyThin)
        {
            for(uint scan = 0; scan < eMaterialPropertyCount; ++scan) {
                scalarAttributeValues[scan] = 0.0f;
                scalarAttributeTextureIndices[scan] = InvalidIndex32;
            }
        }

        eMaterialShader shader;
        uint32 baseColorTextureIndex;
        uint32 normalTextureIndex;
        uint32 flags;
        float3 baseColor;
        float3 transmittanceColor;
        
        float scalarAttributeValues[eMaterialPropertyCount];
        uint32 scalarAttributeTextureIndices[eMaterialPropertyCount];
    };

    struct MeshMetaData
    {
        uint32 indexCount;
        uint32 indexOffset;
        uint32 vertexCount;
        uint32 vertexOffset;
        uint32 materialHash;
        uint32 indicesPerFace;
    };

    struct ModelResourceData
    {
        // -- misc scene info
        AxisAlignedBox  aaBox;
        float4          boundingSphere;

        // -- object counts
        uint32          cameraCount;
        uint32          meshCount;
        uint32          totalVertexCount;
        uint32          indexCount;
        uint32          textureCount;
        uint32          materialCount;

        CameraSettings* cameras;
        FilePathString* textureResourceNames;
        Material*       materials;
        Hash32*         materialHashes;
        MeshMetaData*   meshData;
    };

    struct ModelGeometryData
    {
        uint64 indexSize;
        uint64 faceIndexSize;
        uint64 positionSize;
        uint64 normalsSize;
        uint64 tangentsSize;
        uint64 uvsSize;

        uint32* indices;
        uint32* faceIndexCounts;
        float3* positions;
        float3* normals;
        float4* tangents;
        float2* uvs;
    };

    struct ModelResource
    {
        static cpointer kDataType;
        static cpointer kGeometryDataType;
        static const uint64 kDataVersion;
        static const uint32 kGeometryDataAlignment;

        ModelResourceData* data;
        ModelGeometryData* geometry;

        TextureResource* textures;
        void* rtcDevice;
        void* rtcScene;
        CArray<void*> rtcGeometries;

        Material* defaultMaterial;
        CArray<const Material*> materialLookup;

        ModelResource();
        ~ModelResource();
    };

    void Serialize(CSerializer* serializer, ModelResourceData& data);
    void Serialize(CSerializer* serializer, ModelGeometryData& data);

    Error ReadModelResource(cpointer filepath, ModelResource* scene);
    Error InitializeModelResource(ModelResource* scene);
    void InitializeEmbreeScene(ModelResource* scene);
    void ShutdownModelResource(ModelResource* scene);
}
