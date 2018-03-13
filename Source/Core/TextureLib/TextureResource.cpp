//==============================================================================
// Joe Schutte
//==============================================================================

#include <TextureLib/TextureResource.h>
#include <TextureLib/StbImage.h>
#include <StringLib/FixedString.h>
#include <StringLib/StringUtil.h>
#include <IoLib/BinarySerializer.h>
#include <IoLib/File.h>
#include <SystemLib/BasicTypes.h>

#include <stdio.h>

namespace Shooty
{
    //==============================================================================
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

    //==============================================================================
    void ShutdownTextureResource(TextureResource* texture)
    {
        SafeFreeAligned_(texture->data);
    }

    //==============================================================================
    static void DebugWriteTextureMip(TextureResource* texture, uint level, cpointer filepath)
    {
        uint64 mipOffset = texture->data->mipOffsets[level];
        uint32 mipWidth  = texture->data->mipWidths[level];
        uint32 mipHeight = texture->data->mipHeights[level];
        float3* mip      = &texture->data->mipmaps[mipOffset];

        StbImageWrite(filepath, mipWidth, mipHeight, HDR, (void*)mip);
    }

    //==============================================================================
    void DebugWriteTextureMips(TextureResource* texture, cpointer folder)
    {
        for(uint scan = 0, count = texture->data->mipCount; scan < count; ++scan) {
            FixedString256 path;
            sprintf_s(path.Ascii(), path.Capcaity(), "%s/mip_%llu.hdr", folder, scan);

            DebugWriteTextureMip(texture, scan, path.Ascii());
        }
    }
}