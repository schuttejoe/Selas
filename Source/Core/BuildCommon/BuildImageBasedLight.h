#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    struct ImageBasedLightResourceData;

    bool ImportImageBasedLight(const char* filename, ImageBasedLightResourceData* ibl);
}