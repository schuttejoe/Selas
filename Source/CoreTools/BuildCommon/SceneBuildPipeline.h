#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <ContainersLib\CArray.h>
#include <MathLib\FloatStructs.h>
#include <SystemLib\BasicTypes.h>

namespace Shooty {

    //== Import ====================================================================
    struct ImportedMesh {
        CArray<float3> positions;
        CArray<float3> normals;
        CArray<float2> uv0;

        CArray<uint32> indices;
    };

    struct SceneCamera
    {
        float3 position;
        float3 lookAt;
        float3 up;
        float  fov;
        float  znear;
        float  zfar;
    };

    struct ImportedScene {
        CArray<ImportedMesh*> meshes;
        SceneCamera camera;
    };

    //== Build =====================================================================
    struct BuiltMeshData {
        uint32 indexCount;
        uint32 vertexCount;
        uint32 indexOffset;
        uint32 vertexOffset;
    };

    struct BuiltScene {
        CArray<BuiltMeshData> meshes;
        CArray<uint32>        indices;
        CArray<float3>        positions;
        CArray<float3>        normals;
        CArray<float2>        uv0;
        SceneCamera           camera;
    };
}