#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <ContainersLib/CArray.h>
#include <StringLib/FixedString.h>
#include <MathLib/FloatStructs.h>
#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    #pragma warning(default : 4820)

    //==============================================================================
    // -- Things that are embedded into the scene
    struct MaterialData
    {      
        FixedString256 emissiveTexture;
    };

    //== Import ====================================================================
    struct ImportedMesh
    {
        CArray<float3> positions;
        CArray<float3> normals;
        CArray<float2> uv0;
        CArray<float3> tangents;
        CArray<float3> bitangents;

        CArray<uint32> indices;
        uint32         materialIndex;
    };

    struct SceneCamera
    {
        float3 position;
        float  fov;
        float3 lookAt;
        float  znear;
        float3 up;
        float  zfar;
    };

    struct ImportedScene
    {
        CArray<ImportedMesh*> meshes;
        CArray<FixedString256> materials;
        SceneCamera camera;
    };

    //== Build =====================================================================
    struct BuiltMeshData
    {
        uint32 indexCount;
        uint32 vertexCount;
        uint32 indexOffset;
        uint32 vertexOffset;
    };

    struct VertexAuxiliaryData
    {
        // -- not using float3/float2 here to avoid padding sneaking in.
        float px, py, pz;
        float nx, ny, nz;
        float u, v;
        uint32 materialIndex;
    };

    struct BuiltScene
    {
        // -- Mesh Data
        CArray<BuiltMeshData>       meshes;
        CArray<uint32>              indices;
        CArray<float3>              positions;
        CArray<VertexAuxiliaryData> vertexData;

        // -- Additional scene Data
        CArray<MaterialData> materialData;
        SceneCamera          camera;
    };

}