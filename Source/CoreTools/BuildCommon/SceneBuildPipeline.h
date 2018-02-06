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
        CArray<float4> positions;
        CArray<float3> normals;
        CArray<float2> uv0;

        CArray<uint32> indices;
    };

    struct ImportedScene {
        CArray<ImportedMesh*> meshes;
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
        CArray<float4>        positions;
        CArray<float3>        normals;
        CArray<float2>        uv0;
    };
}