#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SceneLib/EmbreeUtils.h"
#include "TextureLib/TextureCache.h"
#include "GeometryLib/AxisAlignedBox.h"
#include "GeometryLib/Camera.h"
#include "UtilityLib/MurmurHash.h"
#include "StringLib/FixedString.h"
#include "MathLib/FloatStructs.h"
#include "ContainersLib/CArray.h"
#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    #pragma warning(default : 4820)

    struct TextureResource;
    struct HitParameters;

    enum ShaderType
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

    struct MaterialResourceData
    {
        MaterialResourceData()
            : baseColorTextureIndex(InvalidIndex32)
            , flags(0)
            , baseColor(float3::Zero_)
            , transmittanceColor(float3::Zero_)
            , shader(eDisneyThin)
        {
            for(uint scan = 0; scan < eMaterialPropertyCount; ++scan) {
                scalarAttributeValues[scan] = 0.0f;
            }
        }

        ShaderType shader;
        uint32 baseColorTextureIndex;
        uint32 flags;
        float3 baseColor;
        float3 transmittanceColor;
        
        float scalarAttributeValues[eMaterialPropertyCount];
    };

    struct CurveMetaData
    {
        uint32 indexOffset;
        uint32 indexCount;
        Hash32 nameHash;
    };

    struct MeshMetaData
    {
        uint32 indexCount;
        uint32 indexOffset;
        uint32 vertexCount;
        uint32 vertexOffset;
        Hash32 materialHash;
        uint32 indicesPerFace;
        Hash32 meshNameHash;
    };

    struct ModelResourceData
    {
        // -- misc scene info
        AxisAlignedBox  aaBox;
        uint32          totalVertexCount;
        uint32          totalCurveVertexCount;

        CArray<CameraSettings> cameras;
        CArray<FilePathString> textureResourceNames;
        CArray<MaterialResourceData> materials;
        CArray<Hash32>         materialHashes;
        CArray<MeshMetaData>   meshes;
        CArray<CurveMetaData>  curves;
    };

    struct ModelGeometryData
    {
        uint64 indexSize;
        uint64 faceIndexSize;
        uint64 positionSize;
        uint64 normalsSize;
        uint64 tangentsSize;
        uint64 uvsSize;
        uint64 curveIndexSize;
        uint64 curveVertexSize;

        uint32* indices;
        uint32* faceIndexCounts;
        float3* positions;
        float3* normals;
        float4* tangents;
        float2* uvs;
        uint32* curveIndices;
        float4* curveVertices;
    };

    struct ModelResource
    {
        static cpointer kDataType;
        static cpointer kGeometryDataType;
        static const uint64 kDataVersion;
        static const uint32 kGeometryDataAlignment;

        ModelResourceData* data;
        ModelGeometryData* geometry;

        TextureHandle* textureHandles;
        RTCScene rtcScene;
        CArray<GeometryUserData> userDatas;
        CArray<RTCGeometry> rtcGeometries;

        MaterialResourceData* defaultMaterial;

        ModelResource();
        ~ModelResource();
    };

    void Serialize(CSerializer* serializer, CurveMetaData& data);
    void Serialize(CSerializer* serializer, MeshMetaData& data);
    void Serialize(CSerializer* serializer, ModelResourceData& data);
    void Serialize(CSerializer* serializer, ModelGeometryData& data);

    Error ReadModelResource(cpointer filepath, ModelResource* model);
    Error InitializeModelResource(ModelResource* model, TextureCache* textureCache);
    void InitializeEmbreeScene(ModelResource* model, RTCDevice rtcDevice);
    void ShutdownModelResource(ModelResource* model, TextureCache* textureCache);
}
