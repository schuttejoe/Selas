#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    struct TextureResourceData;

    //==============================================================================
    bool BakeTexture(const TextureResourceData* data, cpointer filepath);

}