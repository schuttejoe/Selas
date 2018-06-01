#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/Error.h>
#include <SystemLib/BasicTypes.h>

namespace Selas
{
    struct ImageBasedLightResourceData;

    Error ImportImageBasedLight(const char* filename, ImageBasedLightResourceData* ibl);
}