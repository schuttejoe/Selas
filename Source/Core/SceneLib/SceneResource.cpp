//==============================================================================
// Joe Schutte
//==============================================================================

#include <SceneLib/SceneResource.h>
#include <TextureLib/TextureResource.h>
#include <MathLib/FloatFuncs.h>
#include <MathLib/FloatStructs.h>
#include <IoLib/BinarySerializer.h>
#include <IoLib/File.h>
#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    //==============================================================================
    SceneResource::SceneResource()
        : data(nullptr)
        , textures(nullptr)
    {

    }

    //==============================================================================
    bool ReadSceneResource(cpointer filepath, SceneResource* data)
    {

        void* fileData = nullptr;
        uint32 fileSize = 0;
        ReturnFailure_(File::ReadWholeFile(filepath, &fileData, &fileSize));

        BinaryReader reader;
        SerializerStart(&reader, fileData, fileSize);

        SerializerAttach(&reader, reinterpret_cast<void**>(&data->data), fileSize);
        SerializerEnd(&reader);

        FixupPointerX64(fileData, data->data->textureResourceNames);
        FixupPointerX64(fileData, data->data->materials);
        FixupPointerX64(fileData, data->data->indices);
        FixupPointerX64(fileData, data->data->positions);
        FixupPointerX64(fileData, data->data->vertexData);

        return true;
    }

    //==============================================================================
    bool InitializeSceneResource(SceneResource* scene)
    {
        // -- JSTODO - Should be fetching other resource data from some asset mgr here rather than directly loading it

        uint textureCount = scene->data->textureCount;
        scene->textures = AllocArray_(TextureResource, textureCount);

        for(uint scan = 0, count = scene->data->textureCount; scan < count; ++scan) {
            ReturnFailure_(ReadTextureResource(scene->data->textureResourceNames[scan].Ascii(), &scene->textures[scan]));
        }

        return true;
    }

    //==============================================================================
    void ShutdownSceneResource(SceneResource* scene)
    {
        for(uint scan = 0, count = scene->data->textureCount; scan < count; ++scan) {
            ShutdownTextureResource(&scene->textures[scan]);
        }

        SafeFree_(scene->textures);
        SafeFreeAligned_(scene->data);
    }
}