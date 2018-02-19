//==============================================================================
// Joe Schutte
//==============================================================================

#include <TextureLib/TextureResource.h>
#include <IoLib/BinarySerializer.h>
#include <IoLib/File.h>
#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    bool ReadTextureResource(cpointer filepath, TextureResource* texture)
    {
        void* fileData = nullptr;
        uint32 fileSize = 0;
        ReturnFailure_(File::ReadWholeFile(filepath, &fileData, &fileSize));

        BinaryReader reader;
        SerializerStart(&reader, fileData, fileSize);

        SerializerAttach(&reader, reinterpret_cast<void**>(&texture->data), fileSize);
        SerializerEnd(&reader);

        FixupPointerX64(fileData, texture->data->mipmaps);

        return true;
    }
}