#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "MathLib/FloatStructs.h"

struct RTCSceneTy;
typedef struct RTCSceneTy* RTCScene;

namespace Selas
{
    struct GIIntegrationContext;
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

    struct LightEmissionSample
    {
        float3 position;
        float3 direction;
        float3 radiance;

        // -- probability of choosing that sample point
        float emissionPdfW;
        // -- probability of choosing that sample direction
        float directionPdfA;
        // -- Dot(n', w')
        float cosThetaLight;
    };

    struct LightDirectSample
    {
        float3 direction;
        float3 radiance;
        float distance;

        // -- probability of choosing that sample point
        float emissionPdfW;
        // -- probability of choosing that sample direction
        float directionPdfA;
        // -- Dot(n', w')
        float cosThetaLight;
    };

    float3 IntegrateRectangleLightWithArea(RTCScene& rtcScene, Random::MersenneTwister* twister, const SurfaceParameters& surface, RectangularAreaLight light, uint sampleCount);
    float3 IntegrateRectangleLightWithSolidAngle(RTCScene& rtcScene, Random::MersenneTwister* twister, const SurfaceParameters& surface, RectangularAreaLight light, uint sampleCount);
    float3 IntegrateSphereLightWithAreaSampling(RTCScene& rtcScene, Random::MersenneTwister* twister, const SurfaceParameters& surface, SphericalAreaLight light, uint lightSampleCount);
    float3 IntegrateSphereLightWithSolidAngleSampling(RTCScene& rtcScene, Random::MersenneTwister* twister, const SurfaceParameters& surface, float3 view, SphericalAreaLight light, uint lightSampleCount);

    void EmitIblLightSample(GIIntegrationContext* context, LightEmissionSample& sample);
    void DirectIblLightSample(GIIntegrationContext* context, LightDirectSample& sample);
    float3 IblCalculateRadiance(GIIntegrationContext* context, float3 direction, float& directPdfA, float& emissionPdfW);
    float DirectIblLightPdf(GIIntegrationContext* __restrict context, float3 wi);
}