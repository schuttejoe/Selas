//==============================================================================
// Joe Schutte
//==============================================================================

#include <BuildCommon/BuildScene.h>
#include <BuildCommon/SceneBuildPipeline.h>
#include <SystemLib/CheckedCast.h>
#include <SystemLib/MinMax.h>
#include <SystemLib/MemoryAllocation.h>
#include <MathLib/FloatFuncs.h>
#include <UtilityLib/Color.h>

namespace Shooty {

    #define ReturnFailure_(x) if(!x) { return false; }

    //==============================================================================
    void BuildMeshes(ImportedScene* imported, BuiltScene* built) {
        uint32 totalIndexCount = 0;
        uint32 totalVertexCount = 0;

        for (uint scan = 0, count = imported->meshes.Length(); scan < count; ++scan) {

            ImportedMesh* mesh = imported->meshes[scan];

            BuiltMeshData meshdata;
            meshdata.indexCount = mesh->indices.Length();
            meshdata.vertexCount = mesh->positions.Length();
            meshdata.indexOffset = totalIndexCount;
            meshdata.vertexOffset = totalVertexCount;

            totalIndexCount += meshdata.indexCount;
            totalVertexCount += meshdata.vertexCount;

            built->meshes.Add(meshdata);
            built->indices.Append(mesh->indices);
            built->positions.Append(mesh->positions);
            built->normals.Append(mesh->normals);
            built->uv0.Append(mesh->uv0);
        }
    }

    //==============================================================================
    bool BuildScene(ImportedScene* imported, BuiltScene* built) {
        BuildMeshes(imported, built);

        built->camera = imported->camera;

        return true;
    }
}