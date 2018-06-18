#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "MathLib/FloatStructs.h"

namespace Selas
{
    namespace Math
    {
        float  SrgbToLinearPrecise(float x);
        float3 SrgbToLinearPrecise(float3 srgb);
        float4 SrgbToLinearPrecise(float4 srgb);

        float  LinearToSrgbPrecise(float x);
        float3 LinearToSrgbPrecise(float3 linear);
    }
}