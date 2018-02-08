#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>

namespace Shooty {

    float3 GgxBrdf(float3 n, float3 l, float3 v, float3 albedo, float3 reflectance, float roughness);

}