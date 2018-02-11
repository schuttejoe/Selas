#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/ImportanceSampling.h>
#include <MathLib/FloatStructs.h>
#include <SystemLib/BasicTypes.h>

namespace Shooty {

    struct ImageBasedLightResourceData {
        ImportanceSampling::IblDensityFunctions densityfunctions;
        float3* hdrData;
    };

    struct ImageBasedLightResource {
        ImageBasedLightResourceData* data;
    };

    bool ReadImageBasedLightResource(cpointer filepath, ImageBasedLightResource* resource);
}