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

    struct SceneResourceData
    {
        // -- camera information
        CameraSettings  camera;

        // -- misc scene info
        AxisAlignedBox  aaBox;
        float4          boundingSphere;
        
        // -- only used when no ibl is provided for the scene
        float3          backgroundIntensity;

        // -- object counts
        uint32          meshCount;
        uint32          totalVertexCount;
        uint32          indexCount;
        uint32          textureCount;
        uint32          materialCount;

        FilePathString* textureResourceNames;
        Material*       materials;
        Hash32*         materialHashes;
        MeshMetaData*   meshData;
    };

    struct SceneGeometryData
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

    struct SceneResource
    {
        static cpointer kDataType;
        static cpointer kGeometryDataType;
        static const uint64 kDataVersion;
        static const uint32 kGeometryDataAlignment;

        SceneResourceData* data;
        SceneGeometryData* geometry;

        TextureResource* textures;
        void* rtcDevice;
        void* rtcScene;
        CArray<void*> rtcGeometries;

        Material* defaultMaterial;
        CArray<const Material*> materialLookup;

        SceneResource();
        ~SceneResource();
    };

    void Serialize(CSerializer* serializer, SceneResourceData& data);
    void Serialize(CSerializer* serializer, SceneGeometryData& data);

    Error ReadSceneResource(cpointer filepath, SceneResource* scene);
    Error InitializeSceneResource(SceneResource* scene);
    void InitializeEmbreeScene(SceneResource* scene);
    void ShutdownSceneResource(SceneResource* scene);
}
