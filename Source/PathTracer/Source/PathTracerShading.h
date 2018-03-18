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
    struct SurfaceParameters;
    struct SceneResource;

    float4 ToTangentSpaceQuaternion(float3 n);

    float3 CalculateDirectLighting(SceneResource* scene, Random::MersenneTwister* twister, const SurfaceParameters& surface);

    void ImportanceSampleGgxVdn(Random::MersenneTwister* twister, const SurfaceParameters& surface, float3 wo, float3& wi, float3& reflectance);
    void ImportanceSampleDisneyBrdf(Random::MersenneTwister* twister, const SurfaceParameters& surface, float3 wo, float3& wi, float3& reflectance);
}