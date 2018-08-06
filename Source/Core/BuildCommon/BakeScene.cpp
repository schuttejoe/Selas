//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/BakeScene.h"
#include "BuildCore/BuildContext.h"
#include "IoLib/BinarySerializer.h"

namespace Selas
{
    //=============================================================================================================================
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

    //=============================================================================================================================
    static void SerializeMeshMetaData(BinaryWriter* writer, const BuiltScene& sceneData)
    {
        SerializerWritePointerOffsetX64(writer);
        SerializerWritePointerData(writer, sceneData.meshes.GetData(), sceneData.meshes.DataSize());
    }

    //=============================================================================================================================
    static void SerializeBufferAligned(BinaryWriter* writer, void* data, uint32 dataSize, uint32 pw2alignment)
    {
        SerializerWritePointerOffsetX64(writer);
        SerializerWritePointerData(writer, data, dataSize);

        uint32 alignedSize = (dataSize + pw2alignment - 1) & ~(pw2alignment - 1);
        for(uint scan = dataSize; scan < alignedSize; ++scan) {
            uint8 zero = 0;
            SerializerWritePointerData(writer, &zero, sizeof(uint8));
        }
    }

    //=============================================================================================================================
    static void SerializeGeometry(BinaryWriter* writer, const BuiltScene& sceneData)
    {  
        SerializeBufferAligned(writer, (void*)sceneData.indices.GetData(), sceneData.indices.DataSize(), 
                               SceneResource::kGeometryDataAlignment);
        SerializeBufferAligned(writer, (void*)sceneData.faceIndexCounts.GetData(), sceneData.faceIndexCounts.DataSize(),
                               SceneResource::kGeometryDataAlignment);
        SerializeBufferAligned(writer, (void*)sceneData.positions.GetData(), sceneData.positions.DataSize(),
                               SceneResource::kGeometryDataAlignment);
        SerializeBufferAligned(writer, (void*)sceneData.normals.GetData(), sceneData.normals.DataSize(),
                               SceneResource::kGeometryDataAlignment);
        SerializeBufferAligned(writer, (void*)sceneData.tangents.GetData(), sceneData.tangents.DataSize(),
                               SceneResource::kGeometryDataAlignment);
        SerializeBufferAligned(writer, (void*)sceneData.uvs.GetData(), sceneData.uvs.DataSize(),
                               SceneResource::kGeometryDataAlignment);
    }

    //=============================================================================================================================
    static Error BakeSceneMetaData(BuildProcessorContext* context, const BuiltScene& sceneData)
    {
        uint32 presize = sceneData.textures.DataSize() + sceneData.materials.DataSize();

        BinaryWriter writer;
        SerializerStart(&writer, 0, presize);

        SerializerWrite(&writer, &sceneData.camera, sizeof(sceneData.camera));
        SerializerWrite(&writer, &sceneData.aaBox, sizeof(sceneData.aaBox));
        SerializerWrite(&writer, &sceneData.boundingSphere, sizeof(sceneData.boundingSphere));
        SerializerWrite(&writer, &sceneData.backgroundIntensity, sizeof(sceneData.backgroundIntensity));

        uint32 padding = 0;
        SerializerWrite(&writer, &padding, sizeof(padding));

        SerializeMaterials(&writer, sceneData);
        SerializeMeshMetaData(&writer, sceneData);

        uint32 meshCount = sceneData.meshes.Length();
        uint32 vertexCount = sceneData.positions.Length();
        uint32 indexCounts = sceneData.indices.Length();

        SerializerWrite(&writer, &meshCount, sizeof(meshCount));
        SerializerWrite(&writer, &vertexCount, sizeof(vertexCount));
        SerializerWrite(&writer, &indexCounts, sizeof(indexCounts));

        void* assetData;
        uint32 assetSize;
        ReturnError_(SerializerEnd(&writer, assetData, assetSize));

        ReturnError_(context->CreateOutput(SceneResource::kDataType, SceneResource::kDataVersion, context->source.name.Ascii(),
                                           assetData, assetSize));

        Free_(assetData);

        return Success_;
    }

    //=============================================================================================================================
    static Error BakeSceneGeometryData(BuildProcessorContext* context, const BuiltScene& sceneData)
    {
        uint32 geometryDataSize = 1024/*extra space for alignment*/
                                + sceneData.faceIndexCounts.DataSize()
                                + sceneData.positions.DataSize()
                                + sceneData.normals.DataSize()
                                + sceneData.tangents.DataSize()
                                + sceneData.uvs.DataSize()
                                + sceneData.indices.DataSize();

        BinaryWriter writer;
        SerializerStart(&writer, 0, geometryDataSize);

        SerializeGeometry(&writer, sceneData);

        void* assetData;
        uint32 assetSize;
        ReturnError_(SerializerEnd(&writer, assetData, assetSize));

        ReturnError_(context->CreateOutput(SceneResource::kGeometryDataType, SceneResource::kDataVersion,
                                           context->source.name.Ascii(), assetData, assetSize));

        Free_(assetData);

        return Success_;
    }

    //=============================================================================================================================
    Error BakeScene(BuildProcessorContext* context, const BuiltScene& sceneData)
    {
        ReturnError_(BakeSceneMetaData(context, sceneData));
        ReturnError_(BakeSceneGeometryData(context, sceneData));

        return Success_;
    }
}