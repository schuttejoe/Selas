//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "TextureLib/StbImage.h"
#include "StringLib/StringUtil.h"
#include "SystemLib/MemoryAllocation.h"

#define STBI_MALLOC     Alloc_
#define STBI_REALLOC    Realloc_
#define STBI_FREE       Free_

#define STB_IMAGE_WRITE_IMPLEMENTATION
#if IsWindows_
    #define STBI_MSC_SECURE_CRT
#endif
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace Selas
{
    //=============================================================================================================================
    Error StbImageRead(cpointer filepath, uint requestedChannels, uint nonHdrBitDepth,
                       uint& width, uint& height, uint& channels, bool& floatData, void*& rgba)
    {
        int32 w_;
        int32 h_;
        int32 c_;

        void* raw = nullptr;
        if(StringUtil::EndsWithIgnoreCase(filepath, "hdr")) {
            raw = (void*)stbi_loadf(filepath, &w_, &h_, &c_, (int)requestedChannels);
            floatData = true;
        }
        else {
            if(nonHdrBitDepth == 16) {
                raw = (void*)stbi_load_16(filepath, &w_, &h_, &c_, (int)requestedChannels);
            }
            else if(nonHdrBitDepth == 8) {
                raw = (void*)stbi_load(filepath, &w_, &h_, &c_, (int)requestedChannels);
            }
            else {
                return Error_("Unsupported bit depth");
            }
            
            floatData = false;
        }

        if(raw == nullptr) {
            return Error_("Failed to load texture file: %s", filepath);
        }

        width = (uint)w_;
        height = (uint)h_;
        channels = (uint)c_;
        rgba = raw;

        return Success_;
    }

    //=============================================================================================================================
    Error StbImageWrite(cpointer filepath, uint width, uint height, uint channels, StbImageFormats format, void* rgba)
    {
        int32 ret = 0;
        switch(format) {
        case PNG:
            ret = stbi_write_png(filepath, (int)width, (int)height, (int)channels, rgba, (int)(sizeof(uint32) * width));
            break;
        case BMP:
            ret = stbi_write_bmp(filepath, (int)width, (int)height, (int)channels, rgba);
            break;
        case TGA:
            ret = stbi_write_tga(filepath, (int)width, (int)height, (int)channels, rgba);
            break;
        case HDR:
            ret = stbi_write_hdr(filepath, (int)width, (int)height, (int)channels, (float*)rgba);
            break;
        case JPG:
            ret = stbi_write_jpg(filepath, (int)width, (int)height, (int)channels, rgba, 50);
            break;
        };

        if(ret == 0) {
            return Error_("Failed writing texture to file: %s", filepath);
        }

        return Success_;
    }
}