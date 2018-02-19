#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>
#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    struct TextureResourceData;

    enum TextureMipFilters
    {
        Box,
        Mitchell,
        Lanczos
    };

    bool ImportTexture(const char* filepath, TextureMipFilters prefilter, TextureResourceData* texture);
};