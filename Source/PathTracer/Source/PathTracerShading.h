#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>

struct RTCSceneTy;
typedef struct RTCSceneTy* RTCScene;

namespace Shooty
{
    namespace Random
    {
        struct MersenneTwister;
    }

    struct ImageBasedLightResourceData;

    float3 ImportanceSampleIbl(RTCScene& rtcScene, ImageBasedLightResourceData* ibl, Random::MersenneTwister* twister, float3 p, float3 n, float3 v, float3 albedo, float3 reflectance, float roughness);
    float3 ImportanceSampleGgx(ImageBasedLightResourceData* ibl, Random::MersenneTwister* twister, float3 n, float3 v, float3 albedo, float3 reflectance, float roughness);
}