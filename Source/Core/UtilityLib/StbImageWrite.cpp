//==============================================================================
// Joe Schutte
//==============================================================================

#include <UtilityLib/StbImageWrite.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include <stb_image_write.h>

namespace Shooty {

    //==============================================================================
    bool StbImageWrite(cpointer filepath, uint width, uint height, StbImageFormats format, void* rgba) {

        int32 ret = 0;
        switch (format) {
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
                ret = stbi_write_hdr(filepath, (int)width, (int)height, 4, (float*)rgba);
                break;
            case JPG:
                ret = stbi_write_jpg(filepath, (int)width, (int)height, 4, rgba, 50);
                break;
        };
    
        return ret != 0;
    }
}