#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Selas
{
    struct TextureResourceData;

    //==============================================================================
    bool BakeTexture(const TextureResourceData* data, cpointer filepath);

}