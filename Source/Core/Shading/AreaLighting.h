#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "MathLib/FloatStructs.h"

struct RTCSceneTy;
typedef struct RTCSceneTy* RTCScene;

namespace Selas
{
    class CSampler;
    struct GIIntegratorContext;
    struct SurfaceParameters;

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

    //float3 IntegrateRectangleLightWithArea(RTCScene& rtcScene, CSampler* sampler, const SurfaceParameters& surface,
    //                                       RectangularAreaLight light, uint sampleCount);
    //float3 IntegrateRectangleLightWithSolidAngle(RTCScene& rtcScene, CSampler* sampler, const SurfaceParameters& surface,
    //                                             RectangularAreaLight light, uint sampleCount);
    //float3 IntegrateSphereLightWithAreaSampling(RTCScene& rtcScene, CSampler* sampler, const SurfaceParameters& surface,
    //                                            SphericalAreaLight light, uint lightSampleCount);
    //float3 IntegrateSphereLightWithSolidAngleSampling(RTCScene& rtcScene, CSampler* sampler, const SurfaceParameters& surface,
    //                                                  float3 view, SphericalAreaLight light, uint lightSampleCount);

    void EmitIblLightSample(GIIntegratorContext* context, LightEmissionSample& sample);
    void DirectIblLightSample(GIIntegratorContext* context, LightDirectSample& sample);
    float3 IblCalculateRadiance(GIIntegratorContext* context, float3 direction, float& directPdfA, float& emissionPdfW);

    void NextEventEstimation(GIIntegratorContext* context, LightDirectSample& sample);
    float BackgroundLightingPdf(GIIntegratorContext* context, float3 wi);
    float3 SampleBackground(GIIntegratorContext* context, float3 wi);
    float3 SampleBackgroundMiss(GIIntegratorContext* context, float3 wi);
}