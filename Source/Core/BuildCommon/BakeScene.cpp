//==============================================================================
// Joe Schutte
//==============================================================================

#include <BuildCommon/BakeScene.h>
#include <IoLib/BinarySerializer.h>

namespace Selas
{
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
        uint32 presize = sceneData.textures.DataSize() +  sceneData.materials.DataSize() + sceneData.indices.DataSize() + sceneData.positions.DataSize() + sceneData.vertexData.DataSize();

        BinaryWriter writer;
        ReturnFailure_(SerializerStart(&writer, filepath, 0, presize));

        SerializerWrite(&writer, &sceneData.camera, sizeof(sceneData.camera));
        SerializerWrite(&writer, &sceneData.aaBox, sizeof(sceneData.aaBox));
        SerializerWrite(&writer, &sceneData.boundingSphere, sizeof(sceneData.boundingSphere));

        SerializeMaterials(&writer, sceneData);
        SerializeMeshes(&writer, sceneData);

        ReturnFailure_(SerializerEnd(&writer));

        return true;
    }
}