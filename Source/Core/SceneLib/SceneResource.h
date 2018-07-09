#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

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
        ePreserveRayDifferentials = 1 << 0,
        eHasTextures              = 1 << 1,
        eTransparent              = 1 << 2,
        eAlphaTested              = 1 << 3,
        eDisplacement             = 1 << 4,
        eInvertDisplacement       = 1 << 5
    };

    enum MeshIndexTypes
    {
        eMeshStandard,
        eMeshAlphaTested,
        eMeshDisplaced,
        eMeshAlphaTestedDisplaced,

        eMeshIndexTypeCount
    };

    struct Material
    {
        Material()
            : albedoTextureIndex(InvalidIndex32)
            , roughnessTextureIndex(InvalidIndex32)
            , displacementTextureIndex(InvalidIndex32)
            , normalTextureIndex(InvalidIndex32)
            , specularTextureIndex(InvalidIndex32)
            , metalnessTextureIndex(InvalidIndex32)
            , roughness(0.0f)
            , albedo(0.0f)
            , metalness(0.0f)
            , flags(0)
            , shader(eDisney)
        {
        }

        uint32 albedoTextureIndex;
        uint32 roughnessTextureIndex;
        uint32 displacementTextureIndex;
        uint32 normalTextureIndex;
        uint32 specularTextureIndex;
        uint32 metalnessTextureIndex;
        float  roughness;
        float  albedo;
        float  metalness;
        float  ior;
        float  displacementScale;
        uint32 flags;
        eMaterialShader shader;
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