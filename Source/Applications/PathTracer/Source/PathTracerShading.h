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

    struct SphericalAreaLight
    {
        float3 intensity;
        float3 center;
        float radius;
    };

    struct RectangularAreaLight
    {
        float3 intensity;
        float3 corner;
        float3 eX;
        float3 eZ;
    };

    struct Material;
    struct ImageBasedLightResourceData;
    struct SurfaceParameters;
    struct SceneResource;

    float3 SampleIbl(ImageBasedLightResourceData* ibl, float3 wi);
    float3 CalculateDirectLighting(RTCScene& rtcScene, SceneResource* scene, Random::MersenneTwister* twister, const SurfaceParameters& surface, float3 v);

    void ImportanceSampleGgxVdn(Random::MersenneTwister* twister, const SurfaceParameters& surface, float3 wo, float3& wi, float3& reflectance);
    void ImportanceSampleDisneyBrdf(Random::MersenneTwister* twister, const SurfaceParameters& surface, float3 wo, float currentIor, float3& wi, float3& reflectance, float& ior);
    void ImportanceSampleDisneyBrdfTransparent(Random::MersenneTwister* twister, const SurfaceParameters& surface, float3 wo, float currentIor, float3& wi, float3& reflectance, float& ior);
}