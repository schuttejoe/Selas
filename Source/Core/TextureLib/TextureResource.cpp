//==============================================================================
// Joe Schutte
//==============================================================================

#include "TextureLib/TextureResource.h"
#include "TextureLib/StbImage.h"
#include "Assets/AssetFileUtils.h"
#include "StringLib/FixedString.h"
#include "StringLib/StringUtil.h"
#include "IoLib/BinarySerializer.h"
#include "IoLib/File.h"
#include "IoLib/Directory.h"
#include "SystemLib/BasicTypes.h"

#include <stdio.h>

namespace Selas
{
    cpointer TextureResource::kDataType = "Textures";

    //==============================================================================
    Error ReadTextureResource(cpointer textureName, TextureResource* texture)
    {
        FilePathString filepath;
        AssetFileUtils::AssetFilePath(TextureResource::kDataType, TextureResource::kDataVersion, textureName, filepath);

        void* fileData = nullptr;
        uint32 fileSize = 0;
        ReturnError_(File::ReadWholeFile(filepath.Ascii(), &fileData, &fileSize));

        BinaryReader reader;
        SerializerStart(&reader, fileData, fileSize);

        SerializerAttach(&reader, reinterpret_cast<void**>(&texture->data), fileSize);
        SerializerEnd(&reader);

        FixupPointerX64(fileData, texture->data->texture);
        return Success_;
    }

    //==============================================================================
    void ShutdownTextureResource(TextureResource* texture)
    {
        SafeFreeAligned_(texture->data);
    }

    //==============================================================================
    static void DebugWriteTextureMip(TextureResource* texture, uint level, cpointer filepath)
    {
        uint channels;
        switch(texture->data->type) {
            case TextureResourceData::Float:
                channels = 1;
                break;
            case TextureResourceData::Float3:
                channels = 3;
                break;
            default:
                Assert_(false);
                channels = 1;
        }

        uint64 mipOffset = texture->data->mipOffsets[level];
        uint32 mipWidth  = texture->data->mipWidths[level];
        uint32 mipHeight = texture->data->mipHeights[level];
        void*  mip       = &texture->data->texture[mipOffset];
        StbImageWrite(filepath, mipWidth, mipHeight, channels, HDR, (void*)mip);
    }

    //==============================================================================
    void DebugWriteTextureMips(TextureResource* texture, cpointer folder, cpointer name)
    {
        for(uint scan = 0, count = texture->data->mipCount; scan < count; ++scan) {
            FixedString256 path;
            #if IsWindows_
                sprintf_s(path.Ascii(), path.Capacity(), "%s/%s_mip_%llu.hdr", folder, name, scan);
            #else
                sprintf(path.Ascii(), "%s/%s_mip_%llu.hdr", folder, name, scan);
            #endif

            Directory::EnsureDirectoryExists(path.Ascii());

            DebugWriteTextureMip(texture, scan, path.Ascii());
        }
    }
}