#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    struct ImageBasedLightResourceData;

    //==============================================================================
    bool BakeImageBasedLight(const ImageBasedLightResourceData* data, cpointer filepath);
}
