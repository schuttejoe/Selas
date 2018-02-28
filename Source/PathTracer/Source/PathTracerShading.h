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

    struct Material
    {
        float3 specularColor;
        float3 albedo;
        float  roughness;
    };

    float3 SampleIbl(ImageBasedLightResourceData* ibl, float3 direction);

    void ImportanceSampleIbl(RTCScene& rtcScene, ImageBasedLightResourceData* ibl, Random::MersenneTwister* twister, float3 p, float3 n, float3 v, Material* material,
                             float3& wi, float3& reflectance);
    void ImportanceSampleGgx(Random::MersenneTwister* twister, float3 n, float3 v, Material* material, float3& wi, float3& reflectance);

    void ImportanceSampleLambert(Random::MersenneTwister* twister, float3 n, float3 v, Material* material, float3& wi, float3& reflectance);

    void MIS(RTCScene& rtcScene, ImageBasedLightResourceData* ibl, Random::MersenneTwister* twister, float3 p, float3 wg, float3 v, Material* material, float3& wi, float3& reflectance);
}