#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SceneLib/SceneResource.h"
#include "ContainersLib/CArray.h"
#include "StringLib/FixedString.h"
#include "GeometryLib/AxisAlignedBox.h"
#include "MathLib/FloatStructs.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    #pragma warning(default : 4820)

    //=============================================================================================================================
    struct ImportedMaterialData
    {
        FixedString256 shaderName;
        FilePathString normalTexture;
        FilePathString baseColorTexture;
        float3 baseColor;

        FilePathString scalarAttributeTextures[eMaterialPropertyCount];
        float scalarAttributes[eMaterialPropertyCount];
        
        bool alphaTested;
        bool invertDisplacement;
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
        CameraSettings camera;
    };

    //== Build =====================================================================
    struct BuiltMeshData
    {
        uint32 indexCount;
        uint32 vertexCount;
        uint32 vertexOffset;
    };

    struct BuiltScene
    {
        // -- meta data
        CameraSettings camera;
        AxisAlignedBox aaBox;
        float4 boundingSphere;
        float3 backgroundIntensity;

        // -- material information
        CArray<FilePathString> textures;
        CArray<Material>       materials;

        // -- geometry information
        CArray<BuiltMeshData>       meshes;
        CArray<uint32>              indices[eMeshIndexTypeCount];
        CArray<uint32>              faceIndexCounts;
        CArray<float3>              positions;
        CArray<float3>              normals;
        CArray<float4>              tangents;
        CArray<float2>              uvs;
        CArray<uint32>              materialIndices;
    };

}