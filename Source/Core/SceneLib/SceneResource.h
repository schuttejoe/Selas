#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <StringLib/FixedString.h>
#include <MathLib/FloatStructs.h>
#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    struct TextureResource;

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

    struct SceneResourceData
    {
        Camera         camera;
        uint32         materialCount;
        uint32         pad0;

        MaterialData*  materialData;

        uint32         meshCount;
        uint32         totalIndexCount;
        uint32         totalVertexCount;
        uint32         pad1;

        uint32*        indices;
        float4*        positions;
        float3*        normals;
        float2*        uv0;
        uint16*        materialIndices;
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
}