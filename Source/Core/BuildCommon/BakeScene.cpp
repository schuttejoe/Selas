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
        uint32 materialCount = sceneData.materials.Length();
        uint32 textureCount = sceneData.textures.Length();

        SerializerWrite(writer, &textureCount, sizeof(textureCount));
        SerializerWrite(writer, &materialCount, sizeof(materialCount));

        SerializerWritePointerOffsetX64(writer);
        SerializerWritePointerData(writer, sceneData.textures.GetData(), sceneData.textures.DataSize());

        SerializerWritePointerOffsetX64(writer);
        SerializerWritePointerData(writer, sceneData.materials.GetData(), sceneData.materials.DataSize());
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
        SerializerWritePointerData(writer, sceneData.vertexData.GetData(), sceneData.vertexData.DataSize());
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