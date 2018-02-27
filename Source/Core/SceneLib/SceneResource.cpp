//==============================================================================
// Joe Schutte
//==============================================================================

#include <SceneLib/SceneResource.h>
#include <TextureLib/TextureResource.h>
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

        FixupPointerX64(fileData, data->data->materialData);
        FixupPointerX64(fileData, data->data->indices);
        FixupPointerX64(fileData, data->data->positions);
        FixupPointerX64(fileData, data->data->vertexData);

        return true;
    }

    //==============================================================================
    bool InitializeSceneResource(SceneResource* scene)
    {
        // -- JSTODO - Should be fetching other resource data from some asset mgr here rather than directly loading it
        // -- JSTODO - Texture count from scene asset data

        uint textureCount = 1;
        scene->textures = AllocArray_(TextureResource, textureCount);

        cpointer path = "D:\\Shooty\\ShootyEngine\\_Assets\\Textures\\red_wall_4k";
        ReturnFailure_(ReadTextureResource(path, &scene->textures[0]));

        return true;
    }

    //==============================================================================
    void ShutdownSceneResource(SceneResource* scene)
    {
        SafeFree_(scene->textures);
        SafeFreeAligned_(scene->data);
    }
}