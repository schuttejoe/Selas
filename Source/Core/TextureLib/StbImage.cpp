//==============================================================================
// Joe Schutte
//==============================================================================

#include <TextureLib/StbImage.h>
#include <StringLib/StringUtil.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include <stb_image_write.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace Shooty
{
    //==============================================================================
    bool StbImageRead(cpointer filepath, uint& width, uint& height, uint& channels, void*& rgba)
    {
        int32 w_;
        int32 h_;
        int32 c_;

        void* raw = nullptr;
        if(StringUtil::EndsWithIgnoreCase(filepath, "hdr"))
            raw = (void*)stbi_loadf(filepath, &w_, &h_, &c_, 0);
        else
            raw = (void*)stbi_load(filepath, &w_, &h_, &c_, 0);

        if(raw == nullptr)
            return false;

        width = (uint)w_;
        height = (uint)h_;
        channels = (uint)c_;
        rgba = raw;

        return true;
    }

    //==============================================================================
    bool StbImageWrite(cpointer filepath, uint width, uint height, StbImageFormats format, void* rgba)
    {

        int32 ret = 0;
        switch(format) {
        case PNG:
            ret = stbi_write_png(filepath, (int)width, (int)height, 4, rgba, (int)(sizeof(uint32) * width));
            break;
        case BMP:
            ret = stbi_write_bmp(filepath, (int)width, (int)height, 4, rgba);
            break;
        case TGA:
            ret = stbi_write_tga(filepath, (int)width, (int)height, 4, rgba);
            break;
        case HDR:
            ret = stbi_write_hdr(filepath, (int)width, (int)height, 3, (float*)rgba);
            break;
        case JPG:
            ret = stbi_write_jpg(filepath, (int)width, (int)height, 4, rgba, 50);
            break;
        };

        return ret != 0;
    }
}