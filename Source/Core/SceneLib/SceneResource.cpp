//==============================================================================
// Joe Schutte
//==============================================================================

#include "SceneLib/SceneResource.h"
#include "TextureLib/TextureResource.h"
#include "Assets/AssetFileUtils.h"
#include "MathLib/FloatFuncs.h"
#include "MathLib/FloatStructs.h"
#include "IoLib/BinarySerializer.h"
#include "IoLib/File.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    cpointer SceneResource::kDataType = "Scene";
    cpointer SceneResource::kGeometryDataType = "SceneGeometry";

    const uint64 SceneResource::kDataVersion = 1529639962ul;
    const uint32 SceneResource::kSceneDataAlignment = 16;
    static_assert(sizeof(SceneGeometryData) % SceneResource::kSceneDataAlignment == 0, "SceneGeometryData must be aligned");

    //==============================================================================
    SceneResource::SceneResource()
        : data(nullptr)
        , textures(nullptr)
    {

    }

    //==============================================================================
    static Error ReadSceneMetaData(cpointer assetname, SceneResource* data)
    {
        FilePathString filepath;
        AssetFileUtils::AssetFilePath(SceneResource::kDataType, SceneResource::kDataVersion, assetname, filepath);

        void* fileData = nullptr;
        uint32 fileSize = 0;
        ReturnError_(File::ReadWholeFile(filepath.Ascii(), &fileData, &fileSize));

        BinaryReader reader;
        SerializerStart(&reader, fileData, fileSize);

        SerializerAttach(&reader, reinterpret_cast<void**>(&data->data), fileSize);
        SerializerEnd(&reader);

        FixupPointerX64(fileData, data->data->textureResourceNames);
        FixupPointerX64(fileData, data->data->materials);

        return Success_;
    }

    //==============================================================================
    static Error ReadSceneGeometryData(cpointer assetname, SceneResource* data)
    {
        FilePathString filepath;
        AssetFileUtils::AssetFilePath(SceneResource::kGeometryDataType, SceneResource::kDataVersion, assetname, filepath);

        void* fileData = nullptr;
        uint32 fileSize = 0;
        ReturnError_(File::ReadWholeFile(filepath.Ascii(), &fileData, &fileSize));

        BinaryReader reader;
        SerializerStart(&reader, fileData, fileSize);
        SerializerAttach(&reader, reinterpret_cast<void**>(&data->geometry), fileSize);
        SerializerEnd(&reader);

        for(uint scan = 0; scan < eMeshIndexTypeCount; ++scan) {
            FixupPointerX64(fileData, data->geometry->indices[scan]);
        }
        FixupPointerX64(fileData, data->geometry->faceIndexCounts);
        FixupPointerX64(fileData, data->geometry->positions);
        FixupPointerX64(fileData, data->geometry->normals);
        FixupPointerX64(fileData, data->geometry->tangents);
        FixupPointerX64(fileData, data->geometry->uvs);
        FixupPointerX64(fileData, data->geometry->materialIndices);

        return Success_;
    }

    //==============================================================================
    Error ReadSceneResource(cpointer assetname, SceneResource* data)
    {
        ReturnError_(ReadSceneMetaData(assetname, data));
        ReturnError_(ReadSceneGeometryData(assetname, data));

        return Success_;
    }

    //==============================================================================
    Error InitializeSceneResource(SceneResource* scene)
    {
        // -- JSTODO - Should be fetching other resource data from some asset mgr here rather than directly loading it

        uint textureCount = scene->data->textureCount;
        scene->textures = AllocArray_(TextureResource, textureCount);

        for(uint scan = 0, count = scene->data->textureCount; scan < count; ++scan) {
            ReturnError_(ReadTextureResource(scene->data->textureResourceNames[scan].Ascii(), &scene->textures[scan]));
        }

        return Success_;
    }

    //==============================================================================
    void ShutdownSceneResource(SceneResource* scene)
    {
        for(uint scan = 0, count = scene->data->textureCount; scan < count; ++scan) {
            ShutdownTextureResource(&scene->textures[scan]);
        }

        SafeFree_(scene->textures);
        SafeFreeAligned_(scene->data);
        SafeFreeAligned_(scene->geometry);
    }
}