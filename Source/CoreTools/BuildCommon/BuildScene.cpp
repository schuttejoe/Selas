//==============================================================================
// Joe Schutte
//==============================================================================

#include "BuildScene.h"
#include <SystemLib/CheckedCast.h>
#include <SystemLib/MinMax.h>
#include <SystemLib/MemoryAllocation.h>
#include <MathLib/FloatFuncs.h>
#include <UtilityLib/Color.h>
#include <float.h>

namespace Shooty {

    #define ReturnFailure_(x) if(!x) { return false; }

    //==============================================================================
    bool BuildVertices(ImportedMesh& imported, BuiltMeshData& built) {
        uint vertexCount = imported.positions.Length();
        float4* positions = NewArray_(float4, vertexCount);
        AdditionalVertexData* additionalVertexData = NewArray_(AdditionalVertexData, vertexCount);

        built.vertexCount = CheckedCast<uint32>(vertexCount);
        built.positions = positions;
        built.vertexData = additionalVertexData;

        for (uint scan = 0; scan < vertexCount; ++scan) {
            float3 pos = imported.positions[scan];
            built.positions[scan] = float4(pos, 1.0f);
            built.vertexData[scan].normal = imported.normals[scan];
            built.vertexData[scan].uv0 = imported.uv0[scan];
        }

        return true;
    }

    //==============================================================================
    bool BuildIndices(ImportedMesh& imported, BuiltMeshData& built) {
        uint indexCount = imported.indices.Length();
        built.indices = NewArray_(uint32, indexCount);
        built.indexCount = CheckedCast<uint32>(indexCount);

        for (uint scan = 0; scan < indexCount; ++scan) {
            built.indices[scan] = CheckedCast<uint32>(imported.indices[scan]);
        }

        return true;
    }

    //==============================================================================
    bool BuildMesh(ImportedMesh& imported, BuiltMeshData& built) {
        BuildVertices(imported, built);
        BuildIndices(imported, built);

        return true;
    }

    //==============================================================================
    bool BuildScene(ImportedScene* imported, BuiltScene* built) {
        built->meshes.Resize(imported->meshes.Length());
        for (uint scan = 0, count = imported->meshes.Length(); scan < count; ++scan) {
            built->meshes[scan] = New_(BuiltMeshData);
            ReturnFailure_(BuildMesh(*imported->meshes[scan], *built->meshes[scan]));
        }

        return true;
    }

    //==============================================================================
    static void ShutdownMesh(BuiltMeshData* meshData) {
        SafeDeleteArray_(meshData->positions);
        SafeDeleteArray_(meshData->vertexData);
        SafeDeleteArray_(meshData->indices);

        Delete_(meshData);
    }

    //==============================================================================
    void ShutdownBuiltScene(BuiltScene* built) {
        for (uint scan = 0, meshCount = built->meshes.Length(); scan < meshCount; ++scan) {
            ShutdownMesh(built->meshes[scan]);
        }
        built->meshes.Close();
    }
}