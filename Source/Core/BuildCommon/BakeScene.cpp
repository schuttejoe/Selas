//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/BakeScene.h"
#include "BuildCore/BuildContext.h"
#include "SceneLib/SceneResource.h"

namespace Selas
{
    //=============================================================================================================================
    Error BakeScene(BuildProcessorContext* context, const BuiltScene& sceneData)
    {
        SceneResourceData data;
        data.camera               = sceneData.camera;
        data.aaBox                = sceneData.aaBox;
        data.boundingSphere       = sceneData.boundingSphere;
        data.backgroundIntensity  = sceneData.backgroundIntensity;
        data.meshCount            = sceneData.meshes.Count();
        data.totalVertexCount     = sceneData.positions.Count();
        data.indexCount           = sceneData.indices.Count();
        data.textureCount         = sceneData.textures.Count();
        data.materialCount        = sceneData.materials.Count();
        data.textureResourceNames = (FilePathString*)sceneData.textures.DataPointer();
        data.materials            = (Material*)sceneData.materials.DataPointer();
        data.materialHashes       = (Hash32*)sceneData.materialHashes.DataPointer();
        data.meshData             = (MeshMetaData*)sceneData.meshes.DataPointer();
        
        context->CreateOutput(SceneResource::kDataType, SceneResource::kDataVersion, context->source.name.Ascii(), data);

        SceneGeometryData geometry;
        geometry.indexSize       = sceneData.indices.DataSize();
        geometry.faceIndexSize   = sceneData.faceIndexCounts.DataSize();
        geometry.positionSize    = sceneData.positions.DataSize();
        geometry.normalsSize     = sceneData.normals.DataSize();
        geometry.tangentsSize    = sceneData.tangents.DataSize();
        geometry.uvsSize         = sceneData.uvs.DataSize();
        geometry.indices         = (uint32*)sceneData.indices.DataPointer();
        geometry.faceIndexCounts = (uint32*)sceneData.faceIndexCounts.DataPointer();
        geometry.positions       = (float3*)sceneData.positions.DataPointer();
        geometry.normals         = (float3*)sceneData.normals.DataPointer();
        geometry.tangents        = (float4*)sceneData.tangents.DataPointer();
        geometry.uvs             = (float2*)sceneData.uvs.DataPointer();

        context->CreateOutput(SceneResource::kGeometryDataType, SceneResource::kDataVersion, context->source.name.Ascii(),
                              geometry);

        return Success_;
    }
}