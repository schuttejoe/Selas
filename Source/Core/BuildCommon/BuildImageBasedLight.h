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

    Error ImportImageBasedLight(BuildProcessorContext* context, ImageBasedLightResourceData* ibl);
    Error ImportDualImageBasedLight(BuildProcessorContext* context, cpointer lightPath, cpointer missPath,
                                    ImageBasedLightResourceData* ibl);
}