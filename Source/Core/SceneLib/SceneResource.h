#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>
#include <SystemLib/BasicTypes.h>

namespace Shooty {

    struct SceneMeshData {
        uint32 indexCount;
        uint32 vertexCount;
        uint32 indexOffset;
        uint32 vertexOffset;
    };

    struct Camera
    {
        float3 position;
        float3 lookAt;
        float3 up;
        float  fov;
        float  znear;
        float  zfar;
    };

    struct SceneResourceData {
        Camera         camera;
        uint32         meshCount;
        uint32         totalIndexCount;
        uint32         totalVertexCount;
        uint32         pad;
        SceneMeshData* meshDatas;
        uint32*        indices;
        float4*        positions;
        float3*        normals;
        float2*        uv0;
    };

    struct SceneResource {
        SceneResourceData* data;
    };

    bool ReadSceneResource(cpointer filepath, SceneResource* data);
}