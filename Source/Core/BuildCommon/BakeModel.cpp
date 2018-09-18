//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/BakeModel.h"
#include "BuildCore/BuildContext.h"
#include "SceneLib/ModelResource.h"

namespace Selas
{
    //=============================================================================================================================
    Error BakeModel(BuildProcessorContext* context, cpointer name, const BuiltModel& model)
    {
        ModelResourceData data;
        data.aaBox                = model.aaBox;
        data.totalVertexCount     = (uint32)model.positions.Count();
        data.totalCurveVertexCount     = (uint32)model.curveVertices.Count();
        data.cameras.Append(model.cameras);
        data.textureResourceNames.Append(model.textures);
        data.materials.Append(model.materials);
        data.materialHashes.Append(model.materialHashes);
        data.meshes.Append(model.meshes);
        data.curves.Append(model.curves);
        
        context->CreateOutput(ModelResource::kDataType, ModelResource::kDataVersion, name, data);

        ModelGeometryData geometry;
        geometry.indexSize       = model.indices.DataSize();
        geometry.faceIndexSize   = model.faceIndexCounts.DataSize();
        geometry.positionSize    = model.positions.DataSize();
        geometry.normalsSize     = model.normals.DataSize();
        geometry.tangentsSize    = model.tangents.DataSize();
        geometry.uvsSize         = model.uvs.DataSize();
        geometry.curveIndexSize  = model.curveIndices.DataSize();
        geometry.curveVertexSize = model.curveVertices.DataSize();

        geometry.indices         = (uint32*)model.indices.DataPointer();
        geometry.faceIndexCounts = (uint32*)model.faceIndexCounts.DataPointer();
        geometry.positions       = (float3*)model.positions.DataPointer();
        geometry.normals         = (float3*)model.normals.DataPointer();
        geometry.tangents        = (float4*)model.tangents.DataPointer();
        geometry.uvs             = (float2*)model.uvs.DataPointer();
        geometry.curveIndices    = (uint32*)model.curveIndices.DataPointer();
        geometry.curveVertices   = (float4*)model.curveVertices.DataPointer();

        context->CreateOutput(ModelResource::kGeometryDataType, ModelResource::kDataVersion, name, geometry);

        return Success_;
    }
}