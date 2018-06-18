//==============================================================================
// Joe Schutte
//==============================================================================

#include "BuildCommon/BakeScene.h"
#include "BuildCore/BuildContext.h"
#include "IoLib/BinarySerializer.h"

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
        uint32 solidIndexCount = sceneData.indices.Length();
        uint32 atIndexCount = sceneData.alphaTestedIndices.Length();
        uint32 vertexCount = sceneData.positions.Length();
        
        SerializerWrite(writer, &meshCount, sizeof(meshCount));
        SerializerWrite(writer, &solidIndexCount, sizeof(solidIndexCount));
        SerializerWrite(writer, &atIndexCount, sizeof(atIndexCount));
        SerializerWrite(writer, &vertexCount, sizeof(vertexCount));

        SerializerWritePointerOffsetX64(writer);
        SerializerWritePointerData(writer, sceneData.indices.GetData(), sceneData.indices.DataSize());
        SerializerWritePointerOffsetX64(writer);
        SerializerWritePointerData(writer, sceneData.alphaTestedIndices.GetData(), sceneData.alphaTestedIndices.DataSize());
        SerializerWritePointerOffsetX64(writer);
        SerializerWritePointerData(writer, sceneData.positions.GetData(), sceneData.positions.DataSize());
        SerializerWritePointerOffsetX64(writer);
        SerializerWritePointerData(writer, sceneData.vertexData.GetData(), sceneData.vertexData.DataSize());
    }

    //==============================================================================
    Error BakeScene(BuildProcessorContext* context, const BuiltScene& sceneData)
    {
        uint32 presize = sceneData.textures.DataSize() +  sceneData.materials.DataSize() + sceneData.indices.DataSize() + sceneData.alphaTestedIndices.DataSize() + sceneData.positions.DataSize() + sceneData.vertexData.DataSize();

        BinaryWriter writer;
        SerializerStart(&writer, 0, presize);

        SerializerWrite(&writer, &sceneData.camera, sizeof(sceneData.camera));
        SerializerWrite(&writer, &sceneData.aaBox, sizeof(sceneData.aaBox));
        SerializerWrite(&writer, &sceneData.boundingSphere, sizeof(sceneData.boundingSphere));

        SerializeMaterials(&writer, sceneData);
        SerializeMeshes(&writer, sceneData);

        void* assetData;
        uint32 assetSize;
        ReturnError_(SerializerEnd(&writer, assetData, assetSize));

        ReturnError_(context->CreateOutput(SceneResource::kDataType, SceneResource::kDataVersion, context->source.name.Ascii(), assetData, assetSize));

        Free_(assetData);

        return Success_;
    }
}