#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SceneLib/SceneResource.h>
#include <ContainersLib/CArray.h>
#include <StringLib/FixedString.h>
#include <MathLib/FloatStructs.h>
#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    #pragma warning(default : 4820)

    //==============================================================================
    // -- Things that are embedded into the scene
    struct ImportedMaterialData
    {
        FixedString256 emissiveTexture;
        FixedString256 albedoTexture;
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

    struct ImportedModel
    {
        CArray<ImportedMesh*> meshes;
        CArray<FixedString256> materials;
        Camera camera;
    };

    //== Build =====================================================================
    struct BuiltMeshData
    {
        uint32 indexCount;
        uint32 vertexCount;
        uint32 indexOffset;
        uint32 vertexOffset;
    };

    struct BuiltScene
    {
        // -- meta data
        Camera camera;

        // -- material information
        CArray<FixedString256> textures;
        CArray<Material>       materials;

        // -- geometry information
        CArray<BuiltMeshData>       meshes;
        CArray<uint32>              indices;
        CArray<float3>              positions;
        CArray<VertexAuxiliaryData> vertexData;
    };

}