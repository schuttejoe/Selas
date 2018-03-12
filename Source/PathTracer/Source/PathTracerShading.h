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

    struct Material;
    struct ImageBasedLightResourceData;

    float3 SampleIbl(ImageBasedLightResourceData* ibl, float3 direction);

    void ImportanceSampleIbl(RTCScene& rtcScene, ImageBasedLightResourceData* ibl, Random::MersenneTwister* twister, float3 p, float3 n, float3 v, Material* material,
                             float3& wi, float3& reflectance);
    void ImportanceSampleGgxD(Random::MersenneTwister* twister, float3 n, float3 v, Material* material, float3& wi, float3& reflectance);
    void ImportanceSampleGgxVdn(Random::MersenneTwister* twister, float3 wg, float3 v, Material* material, float3& wi, float3& reflectance);

    void ImportanceSampleLambert(Random::MersenneTwister* twister, float3 n, float3 v, Material* material, float3& wi, float3& reflectance);

    
}