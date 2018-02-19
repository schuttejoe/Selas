//==============================================================================
// Joe Schutte
//==============================================================================

#include <BuildCommon/BakeScene.h>
#include <IoLib/BinarySerializer.h>

namespace Shooty
{
    //==============================================================================
    static void SerializeCamera(BinaryWriter* writer, const BuiltScene& sceneData)
    {
        SerializerWrite(writer, &sceneData.camera, sizeof(sceneData.camera));
    }

    //==============================================================================
    static void SerializeMaterials(BinaryWriter* writer, const BuiltScene& sceneData)
    {
        uint32 materialCount = sceneData.materialData.Length();
        uint32 pad = 0;

        SerializerWrite(writer, &materialCount, sizeof(materialCount));
        SerializerWrite(writer, &pad, sizeof(pad));

        SerializerWritePointerOffsetX64(writer);
        SerializerWritePointerData(writer, sceneData.materialData.GetData(), sceneData.materialData.DataSize());
    }

    //==============================================================================
    static void SerializeMeshes(BinaryWriter* writer, const BuiltScene& sceneData)
    {

        uint32 meshCount = sceneData.meshes.Length();
        uint32 indexCount = sceneData.indices.Length();
        uint32 vertexCount = sceneData.positions.Length();
        uint32 pad = 0;
        SerializerWrite(writer, &meshCount, sizeof(meshCount));
        SerializerWrite(writer, &indexCount, sizeof(indexCount));
        SerializerWrite(writer, &vertexCount, sizeof(vertexCount));
        SerializerWrite(writer, &pad, sizeof(pad));

        SerializerWritePointerOffsetX64(writer);
        SerializerWritePointerData(writer, sceneData.indices.GetData(), sceneData.indices.DataSize());
        SerializerWritePointerOffsetX64(writer);
        SerializerWritePointerData(writer, sceneData.positions.GetData(), sceneData.positions.DataSize());
        SerializerWritePointerOffsetX64(writer);
        SerializerWritePointerData(writer, sceneData.normals.GetData(), sceneData.normals.DataSize());
        SerializerWritePointerOffsetX64(writer);
        SerializerWritePointerData(writer, sceneData.uv0.GetData(), sceneData.uv0.DataSize());
        SerializerWritePointerOffsetX64(writer);
        SerializerWritePointerData(writer, sceneData.materialIndices.GetData(), sceneData.materialIndices.DataSize());
    }

    //==============================================================================
    bool BakeScene(const BuiltScene& sceneData, cpointer filepath)
    {
        BinaryWriter writer;
        ReturnFailure_(SerializerStart(&writer, filepath));

        SerializeCamera(&writer, sceneData);
        SerializeMaterials(&writer, sceneData);
        SerializeMeshes(&writer, sceneData);

        ReturnFailure_(SerializerEnd(&writer));

        return true;
    }
}