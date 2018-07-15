#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "StringLib/FixedString.h"
#include "GeometryLib/AxisAlignedBox.h"
#include "GeometryLib/Camera.h"
#include "MathLib/FloatStructs.h"
#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    #pragma warning(default : 4820)

    struct TextureResource;
    struct HitParameters;

    enum eMaterialShader
    {
        eDisney,
        eTransparentGgx,

        eShaderCount
    };

    enum eMaterialFlags
    {
        eTransparent              = 1 << 0,
        eAlphaTested              = 1 << 1,
        eDisplacementEnabled      = 1 << 2,
        eInvertDisplacement       = 1 << 3
    };

    enum MeshIndexTypes
    {
        eMeshStandard,
        eMeshAlphaTested,
        eMeshDisplaced,
        eMeshAlphaTestedDisplaced,

        eMeshIndexTypeCount
    };

    enum ScalarMaterialProperties
    {
        // -- Disney BSDF parameters
        eSubsurface,
        eMetallic,
        eSpecular,
        eSpecularTint,
        eRoughness,
        eAnisotropic,
        eSheen,
        eSheenTint,
        eClearcoat,
        eClearcoatGloss,
        eSpecTrans,
        eIor,

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
            , shader(eDisney)
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
        
        float scalarAttributeValues[eMaterialPropertyCount];
        uint32 scalarAttributeTextureIndices[eMaterialPropertyCount];
    };

    struct SceneMetaData
    {
        // -- camera information
        CameraSettings  camera;

        // -- misc scene info
        AxisAlignedBox  aaBox;
        float4          boundingSphere;

        // -- material information
        uint32          textureCount;
        uint32          materialCount;
        // -- long run plan is to have texture header in the scene and then the texture data will be loaded in via caching.
        // -- for now I'm just making textures as a separate resource.
        FilePathString* textureResourceNames;
        Material*       materials;

        // -- mesh information
        uint32          meshCount;
        uint32          totalVertexCount;
        uint32          indexCounts[eMeshIndexTypeCount];
    };

    struct SceneGeometryData
    {
        uint32* indices[eMeshIndexTypeCount];
        uint32* faceIndexCounts;
        float3* positions;
        float3* normals;
        float4* tangents;
        float2* uvs;
        uint32* materialIndices;
    };

    struct SceneResource
    {
        static cpointer kDataType;
        static cpointer kGeometryDataType;
        static const uint64 kDataVersion;
        static const uint32 kSceneDataAlignment;

        SceneMetaData* data;
        SceneGeometryData* geometry;

        TextureResource* textures;
        void* rtcDevice;
        void* rtcScene;
        void* rtcGeometries[eMeshIndexTypeCount];

        SceneResource();
        ~SceneResource();
    };

    Error ReadSceneResource(cpointer filepath, SceneResource* scene);
    Error InitializeSceneResource(SceneResource* scene);
    void InitializeEmbreeScene(SceneResource* scene);
    void ShutdownSceneResource(SceneResource* scene);
}