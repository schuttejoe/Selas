#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <StringLib/FixedString.h>
#include <GeometryLib/AxisAlignedBox.h>
#include <GeometryLib/Camera.h>
#include <MathLib/FloatStructs.h>
#include <SystemLib/BasicTypes.h>

namespace Shooty
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
        eTransparent              = 1 << 2
    };

    struct Material
    {
        Material()
            : emissiveTextureIndex(InvalidIndex32)
            , albedoTextureIndex(InvalidIndex32)
            , roughnessTextureIndex(InvalidIndex32)
            , heightTextureIndex(InvalidIndex32)
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

        uint32 emissiveTextureIndex;
        uint32 albedoTextureIndex;
        uint32 roughnessTextureIndex;
        uint32 heightTextureIndex;
        uint32 normalTextureIndex;
        uint32 specularTextureIndex;
        uint32 metalnessTextureIndex;
        float  roughness;
        float  albedo;
        float  metalness;
        float  ior;
        uint32 flags;
        eMaterialShader shader;
    };

    struct VertexAuxiliaryData
    {
        float px, py, pz;
        float nx, ny, nz;
        float tx, ty, tz, bh;
        float u, v;
        uint32 materialIndex;
    };

    struct SceneResourceData
    {
        // -- camera information
        CameraSettings       camera;

        // -- misc scene info
        AxisAlignedBox       aaBox;
        float4               boundingSphere;

        // -- material information
        uint32               textureCount;
        uint32               materialCount;
        // -- long run plan is to have texture header in the scene and then the texture data will be loaded in via caching.
        // -- for now I'm just making textures as a separate resource.
        FixedString256*      textureResourceNames; 
        Material*            materials;
                             
        // -- mesh information
        uint32               meshCount;
        uint32               totalIndexCount;
        uint32               totalVertexCount;
        uint32               pad0;
                             
        uint32*              indices;
        float4*              positions;
        VertexAuxiliaryData* vertexData;
    };

    struct SceneResource
    {
        SceneResourceData* data;
        TextureResource* textures;

        SceneResource();
    };

    bool ReadSceneResource(cpointer filepath, SceneResource* scene);
    bool InitializeSceneResource(SceneResource* scene);
    void ShutdownSceneResource(SceneResource* scene);

    void CalculateSurfaceParams(const SceneResourceData* scene, uint32 primitiveId, float2 barycentric, HitParameters& hitParameters);
}