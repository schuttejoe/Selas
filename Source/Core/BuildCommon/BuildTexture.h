#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>
#include <SystemLib/Error.h>
#include <SystemLib/BasicTypes.h>

namespace Selas
{
    struct TextureResourceData;

    enum TextureMipFilters
    {
        Box
        //Mitchell,
        //Lanczos
    };

    Error ImportTexture(const char* filepath, TextureMipFilters prefilter, TextureResourceData* texture);
}