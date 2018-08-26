#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SceneLib/ModelResource.h"
#include "GeometryLib/AxisAlignedBox.h"
#include "UtilityLib/MurmurHash.h"
#include "StringLib/FixedString.h"
#include "MathLib/FloatStructs.h"
#include "ContainersLib/CArray.h"
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
        float3 transmittanceColor;

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

        CArray<uint32> triindices;
        CArray<uint32> quadindices;
        Hash32         materialHash;
        Hash32         meshNameHash;
    };

    struct ImportedModel
    {
        CArray<ImportedMesh*> meshes;
        CArray<FixedString256> materials;
        CArray<Hash32> materialHashes;
        CArray<CameraSettings> cameras;
    };

    //== Build =====================================================================
    struct BuiltModel
    {
        // -- meta data
        CArray<CameraSettings> cameras;
        AxisAlignedBox aaBox;
        float4 boundingSphere;
        float3 backgroundIntensity;

        // -- material information
        CArray<FilePathString> textures;
        CArray<Hash32>         materialHashes;
        CArray<Material>       materials;

        // -- geometry information
        CArray<MeshMetaData>        meshes;
        CArray<uint32>              indices;
        CArray<uint32>              faceIndexCounts;
        CArray<float3>              positions;
        CArray<float3>              normals;
        CArray<float4>              tangents;
        CArray<float2>              uvs;
    };

}