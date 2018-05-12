#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>

struct RTCSceneTy;
typedef struct RTCSceneTy* RTCScene;

namespace Shooty
{
    struct SurfaceParameters;
    namespace Random
    {
        struct MersenneTwister;
    };

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

    float3 IntegrateRectangleLightWithArea(RTCScene& rtcScene, Random::MersenneTwister* twister, const SurfaceParameters& surface, RectangularAreaLight light, uint sampleCount);
    float3 IntegrateRectangleLightWithSolidAngle(RTCScene& rtcScene, Random::MersenneTwister* twister, const SurfaceParameters& surface, RectangularAreaLight light, uint sampleCount);
    float3 IntegrateSphereLightWithAreaSampling(RTCScene& rtcScene, Random::MersenneTwister* twister, const SurfaceParameters& surface, SphericalAreaLight light, uint lightSampleCount);
    float3 IntegrateSphereLightWithSolidAngleSampling(RTCScene& rtcScene, Random::MersenneTwister* twister, const SurfaceParameters& surface, float3 view, SphericalAreaLight light, uint lightSampleCount);
}