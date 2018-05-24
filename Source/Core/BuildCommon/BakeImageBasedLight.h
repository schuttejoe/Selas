#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Selas
{
    struct ImageBasedLightResourceData;

    //==============================================================================
    bool BakeImageBasedLight(const ImageBasedLightResourceData* data, cpointer filepath);
}
