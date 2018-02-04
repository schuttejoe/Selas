//==============================================================================
// Joe Schutte
//==============================================================================

#include "BakeScene.h"
#include <IoLib\BinarySerializer.h>

namespace Shooty {

    #define ReturnFailure_(code) if (!code) { return false; }

    //==============================================================================
    bool SerializeMesh(BinaryWriter* serializer, const BuiltMeshData& mesh) {
        uint32 indexCount = mesh.indexCount;
        uint32 vertexCount = mesh.vertexCount;
        SerializerWrite(serializer, &indexCount, sizeof(uint32));
        SerializerWrite(serializer, &vertexCount, sizeof(uint32));

        SerializerWritePointerOffsetX64(serializer);
        SerializerWritePointerData(serializer, mesh.positions, vertexCount * sizeof(float4));

        SerializerWritePointerOffsetX64(serializer);
        SerializerWritePointerData(serializer, mesh.indices, indexCount * sizeof(uint16));

        SerializerWritePointerOffsetX64(serializer);
        SerializerWritePointerData(serializer, mesh.vertexData, vertexCount * sizeof(float3));

        return true;
    }

    //==============================================================================
    bool BakeScene(const BuiltScene& sceneData, cpointer filepath) {
        BinaryWriter serializer;
        ReturnFailure_(SerializerStart(&serializer, filepath));

        uint32 meshCount = sceneData.meshes.Length();

        SerializerWrite(&serializer, &meshCount, sizeof(uint32));

        for (uint scan = 0; scan < meshCount; ++scan) {
            ReturnFailure_(SerializeMesh(&serializer, *sceneData.meshes[scan]));
        }

        ReturnFailure_(SerializerEnd(&serializer));

        return true;
    }
}