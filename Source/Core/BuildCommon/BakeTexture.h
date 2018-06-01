#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/Error.h>
#include <SystemLib/BasicTypes.h>

namespace Selas
{
    struct TextureResourceData;

    //==============================================================================
    Error BakeTexture(const TextureResourceData* data, cpointer filepath);

}