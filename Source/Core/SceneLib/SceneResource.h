#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <StringLib/FixedString.h>
#include <MathLib/FloatStructs.h>
#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    #pragma warning(default : 4820)

    struct TextureResource;
    struct HitParameters;

    struct Camera
    {
        float3 position;
        float  fov;
        float3 lookAt;
        float  znear;
        float3 up;
        float  zfar;
    };

    struct MaterialData
    {
        FixedString256 emissiveTexture;
    };

    enum eMaterialFlags
    {
        ePreserveRayDifferentials = 1 << 0,
        eHasReflectance           = 1 << 1,
        eHasTextures              = 1 << 2
    };

    struct Material
    {
        float3 specularColor;
        float  roughness;

        float  metalness;
        uint32 flags;
        uint32 emissiveTextureIndex;
        uint32 albedoTextureIndex;
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
        Camera               camera;

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