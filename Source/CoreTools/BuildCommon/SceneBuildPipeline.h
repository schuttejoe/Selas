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

    struct ImportedScene {
        CArray<ImportedMesh*> meshes;
    };

    //== Build =====================================================================

    struct AdditionalVertexData {
        float3 normal;
        float2 uv0;
    };

    struct BuiltMeshData {
        uint32 indexCount;
        uint32 vertexCount;

        float4*               positions;
        AdditionalVertexData* vertexData;
        uint32*               indices;
    };

    struct BuiltScene {
        CArray<BuiltMeshData*> meshes;
    };
}