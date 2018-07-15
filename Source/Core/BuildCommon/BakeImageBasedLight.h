#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    struct ImageBasedLightResourceData;
    struct BuildProcessorContext;

    //=============================================================================================================================
    Error BakeImageBasedLight(BuildProcessorContext* context, const ImageBasedLightResourceData* data);
}
