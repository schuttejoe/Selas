#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/Error.h>
#include <SystemLib/BasicTypes.h>

namespace Selas
{
    enum StbImageFormats
    {
        PNG,
        BMP,
        TGA,
        HDR,
        JPG
    };

    Error StbImageRead(cpointer filepath, uint requestedChannels, uint& width, uint& height, uint& channels, void*& rgba);
    Error StbImageWrite(cpointer filepath, uint width, uint height, uint channels, StbImageFormats format, void* rgba);
}