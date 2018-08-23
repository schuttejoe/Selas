//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/BakeModel.h"
#include "BuildCore/BuildContext.h"
#include "SceneLib/ModelResource.h"

namespace Selas
{
    //=============================================================================================================================
    Error BakeModel(BuildProcessorContext* context, const BuiltModel& model)
    {
        ModelResourceData data;
        data.camera               = model.camera;
        data.aaBox                = model.aaBox;
        data.boundingSphere       = model.boundingSphere;
        data.backgroundIntensity  = model.backgroundIntensity;
        data.meshCount            = model.meshes.Count();
        data.totalVertexCount     = model.positions.Count();
        data.indexCount           = model.indices.Count();
        data.textureCount         = model.textures.Count();
        data.materialCount        = model.materials.Count();
        data.textureResourceNames = (FilePathString*)model.textures.DataPointer();
        data.materials            = (Material*)model.materials.DataPointer();
        data.materialHashes       = (Hash32*)model.materialHashes.DataPointer();
        data.meshData             = (MeshMetaData*)model.meshes.DataPointer();
        
        context->CreateOutput(ModelResource::kDataType, ModelResource::kDataVersion, context->source.name.Ascii(), data);

        ModelGeometryData geometry;
        geometry.indexSize       = model.indices.DataSize();
        geometry.faceIndexSize   = model.faceIndexCounts.DataSize();
        geometry.positionSize    = model.positions.DataSize();
        geometry.normalsSize     = model.normals.DataSize();
        geometry.tangentsSize    = model.tangents.DataSize();
        geometry.uvsSize         = model.uvs.DataSize();
        geometry.indices         = (uint32*)model.indices.DataPointer();
        geometry.faceIndexCounts = (uint32*)model.faceIndexCounts.DataPointer();
        geometry.positions       = (float3*)model.positions.DataPointer();
        geometry.normals         = (float3*)model.normals.DataPointer();
        geometry.tangents        = (float4*)model.tangents.DataPointer();
        geometry.uvs             = (float2*)model.uvs.DataPointer();

        context->CreateOutput(ModelResource::kGeometryDataType, ModelResource::kDataVersion, context->source.name.Ascii(),
                              geometry);

        return Success_;
    }
}