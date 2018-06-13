#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "MathLib/FloatStructs.h"
#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    struct TextureResourceData;
    struct BuildProcessorContext;

    enum TextureMipFilters
    {
        Box
        //Mitchell,
        //Lanczos
    };

    Error ImportTexture(BuildProcessorContext* context, TextureMipFilters prefilter, TextureResourceData* texture);
}