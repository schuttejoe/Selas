#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    #define NoComponentCountRequest_ 0

    enum StbImageFormats
    {
        PNG,
        BMP,
        TGA,
        HDR,
        JPG
    };

    Error StbImageRead(cpointer filepath, uint requestedChannels, uint nonHdrBitDepth,
                       uint& width, uint& height, uint& channels, bool& floatData, void*& rgba);
    Error StbImageWrite(cpointer filepath, uint width, uint height, uint channels, StbImageFormats format, void* rgba);
}