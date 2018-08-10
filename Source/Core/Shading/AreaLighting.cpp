
//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "Shading/AreaLighting.h"
#include "Shading/SurfaceParameters.h"
#include "Shading/SurfaceScattering.h"
#include "Shading/IntegratorContexts.h"

#include "SceneLib/SceneResource.h"
#include "GeometryLib/RectangulerLightSampler.h"
#include "GeometryLib/CoordinateSystem.h"
#include "GeometryLib/Disc.h"
#include "MathLib/FloatFuncs.h"
#include "MathLib/Trigonometric.h"
#include "MathLib/ImportanceSampling.h"
#include "MathLib/Random.h"
#include "MathLib/Projection.h"
#include "MathLib/Quaternion.h"
#include "MathLib/GeometryIntersection.h"
#include "SystemLib/MinMax.h"

namespace Selas
{
    #define MaxBounceCount_    10
    #define SkyIntensityScale_ 0.5f

    //=============================================================================================================================
    //float3 IntegrateRectangleLightWithArea(RTCScene& rtcScene, CSampler* sampler, const SurfaceParameters& surface,
    //                                       RectangularAreaLight light, uint sampleCount)
    //{
    //    float3 eX = light.eX;
    //    float3 eZ = light.eZ;
    //    float3 s = light.corner;

    //    float eXLength = Length(eX);
    //    float eZLength = Length(eZ);

    //    float3 lightFacing = Normalize(Cross(eX, eZ));

    //    if(Dot(-lightFacing, surface.perturbedNormal) <= 0.0f) {
    //        return float3::Zero_;
    //    }

    //    float3 Lo = float3::Zero_;

    //    float pdf = 1.0f / (eXLength * eZLength);

    //    for(uint scan = 0; scan < sampleCount; ++scan) {
    //        float u = sampler->UniformFloat();
    //        float v = sampler->UniformFloat();

    //        float3 position = s + u * eX + v * eZ;

    //        float3 ul = position - surface.position;
    //        float distSquared = LengthSquared(ul);
    //        float dist = Math::Sqrtf(distSquared);
    //        float3 l = (1.0f / dist) * ul;

    //        float dotNL = Saturate(Dot(surface.perturbedNormal, l));
    //        float dotSL = Saturate(Dot(lightFacing, -l));

    //        if(dotNL > 0.0f && dotSL > 0.0f && OcclusionRay(rtcScene, surface, l, dist)) {
    //            Lo += (dotNL * dotSL / distSquared) * light.intensity;
    //        }
    //    }

    //    return Lo * (1.0f / (pdf * sampleCount));
    //}

    ////=============================================================================================================================
    //float3 IntegrateRectangleLightWithSolidAngle(RTCScene& rtcScene, CSampler* sampler, const SurfaceParameters& surface,
    //                                             RectangularAreaLight light, uint sampleCount)
    //{
    //    float3 eX = light.eX;
    //    float3 eZ = light.eZ;
    //    float3 s = light.corner;

    //    RectangleLightSampler quadsampler;
    //    InitializeRectangleLightSampler(s, eX, eZ, surface.position, quadsampler);

    //    float3 lightFacing = quadsampler.z;

    //    float3 Lo = float3::Zero_;

    //    float pdf = 1.0f / quadsampler.S;

    //    for(uint scan = 0; scan < sampleCount; ++scan) {
    //        float u = sampler->UniformFloat();
    //        float v = sampler->UniformFloat();

    //        float3 position = SampleRectangleLight(quadsampler, u, v);

    //        float3 ul = position - surface.position;
    //        float distSquared = LengthSquared(ul);
    //        float dist = Math::Sqrtf(distSquared);
    //        float3 l = (1.0f / dist) * ul;

    //        float dotNL = Saturate(Dot(surface.perturbedNormal, l));
    //        float dotSL = Saturate(Dot(lightFacing, -l));

    //        if(dotNL > 0.0f && dotSL > 0.0f && OcclusionRay(rtcScene, surface, l, dist)) {
    //            // -- the dist^2 and Dot(w', n') terms from the pdf and the area form of the rendering equation cancel out
    //            // -- the pdf is constant so that is applied below.
    //            Lo += dotNL * light.intensity;
    //        }
    //    }

    //    return Lo * (1.0f / (pdf * sampleCount));
    //}

    ////=============================================================================================================================
    //float3 IntegrateSphereLightWithAreaSampling(RTCScene& rtcScene, CSampler* sampler, const SurfaceParameters& surface,
    //                                            SphericalAreaLight light, uint lightSampleCount)
    //{
    //    float3 L = light.intensity;
    //    float3 c = light.center;
    //    float  r = light.radius;

    //    float3 Lo = float3::Zero_;

    //    float pdf = 1.0f / (4.0f * Math::Pi_ * r * r);

    //    for(uint scan = 0; scan < lightSampleCount; ++scan) {
    //        float r0 = sampler->UniformFloat();
    //        float r1 = sampler->UniformFloat();

    //        float theta = Math::Acosf(1 - 2.0f * r0);
    //        float phi = Math::TwoPi_ * r1;

    //        float3 sn = Math::SphericalToCartesian(theta, phi);
    //        float3 xp = c + r * sn;

    //        float3 w = xp - surface.position;
    //        float distSquared = Dot(w, w);
    //        float dist = Math::Sqrtf(distSquared);
    //        w = (1.0f / dist) * w;

    //        float dotSL = Dot(sn, -w);
    //        float dotNL = Dot(surface.perturbedNormal, w);

    //        if(dotSL > 0.0f && dotNL > 0.0f && OcclusionRay(rtcScene, surface, w, dist)) {
    //            float bsdf = dotNL;
    //            Lo += L * bsdf * dotSL * (1.0f / distSquared);
    //        }
    //    }

    //    return Lo * (1.0f / (lightSampleCount * pdf));
    //}

    ////=============================================================================================================================
    //float3 IntegrateSphereLightWithSolidAngleSampling(RTCScene& rtcScene, CSampler* sampler, const SurfaceParameters& surface,
    //                                                  float3 view, SphericalAreaLight light, uint lightSampleCount)
    //{
    //    float3 L = light.intensity;
    //    float3 c = light.center;
    //    float  r = light.radius;

    //    float3 w = c - surface.position;
    //    float distanceToCenter = Length(w);
    //    w = w * (1.0f / distanceToCenter);

    //    float q = Math::Sqrtf(1.0f - (r / distanceToCenter) * (r / distanceToCenter));

    //    float3 v, u;
    //    MakeOrthogonalCoordinateSystem(w, &v, &u);

    //    float3x3 toWorld = MakeFloat3x3(u, w, v);

    //    float3 Lo = float3::Zero_;

    //    for(uint scan = 0; scan < lightSampleCount; ++scan) {
    //        float r0 = sampler->UniformFloat();
    //        float r1 = sampler->UniformFloat();

    //        float theta = Math::Acosf(1 - r0 + r0 * q);
    //        float phi = Math::TwoPi_ * r1;

    //        float3 nwp = MatrixMultiply(Math::SphericalToCartesian(theta, phi), toWorld);
    //        float3 wp = -nwp;

    //        float3 xp;
    //        Intersection::RaySphereNearest(surface.position, nwp, c, r, xp);

    //        float distSquared = LengthSquared(xp - surface.position);
    //        float dist = Math::Sqrtf(distSquared);

    //        float dotNL = Saturate(Dot(nwp, surface.perturbedNormal));
    //        if(dotNL > 0.0f && OcclusionRay(rtcScene, surface, nwp, dist)) {
    //            // -- the dist^2 and Dot(w', n') terms from the pdf and the area form of the rendering equation cancel out
    //            float pdf_xp = 1.0f / (Math::TwoPi_ * (1.0f - q));
    //            float bsdfForwardPdf;
    //            float bsdfReversePdf;
    //            float3 bsdf = EvaluateBsdf(surface, view, nwp, bsdfForwardPdf, bsdfReversePdf);
    //            Lo += bsdf * (1.0f / pdf_xp) * L;
    //        }
    //    }

    //    return Lo * (1.0f / lightSampleCount);
    //}

    //=============================================================================================================================
    void EmitIblLightSample(GIIntegrationContext* __restrict context, LightEmissionSample& sample)
    {
        // -- http://www.iliyan.com/publications/ImplementingVCM/ImplementingVCM_TechRep2012_rev2.pdf
        // -- see section 5.1 of ^ to understand the position and emission pdf calculation

        // -- choose direction to sample the ibl
        float r0 = context->sampler.UniformFloat();
        float r1 = context->sampler.UniformFloat();
        float r2 = context->sampler.UniformFloat();
        float r3 = context->sampler.UniformFloat();

        uint x;
        uint y;
        float dirPhi;
        float dirTheta;

        // -- Importance sample the ibl. Note that we're cheating and treating the sample pdf as an area measure
        // -- even though it's a solid angle measure.
        Ibl(&context->sceneData->ibl->densityfunctions, r0, r1, dirTheta, dirPhi, x, y, sample.directionPdfA);
        float3 toIbl = Math::SphericalToCartesian(dirTheta, dirPhi);
        float3 radiance = SampleIbl(context->sceneData->ibl, x, y);

        float3 dX, dZ;
        MakeOrthogonalCoordinateSystem(toIbl, &dX, &dZ);

        float sceneBoundingRadius = context->sceneData->scene->data->boundingSphere.w;
        float3 sceneCenter = context->sceneData->scene->data->boundingSphere.XYZ();

        float2 discSample = SampleConcentricDisc(r2, r3);
        float discPdf = ConcentricDiscPdf();

        float3 position = sceneCenter + sceneBoundingRadius * (toIbl + discSample.x * dX + discSample.y * dZ);

        float pdfPosition = discPdf * (1.0f / (sceneBoundingRadius * sceneBoundingRadius));
        float pdfDirection = sample.directionPdfA;

        sample.position = position;
        sample.direction = -toIbl;
        sample.radiance = radiance;
        sample.emissionPdfW = pdfPosition * pdfDirection;
        sample.cosThetaLight = 1.0f; // -- not used for ibl light sources
    }

    //=============================================================================================================================
    void DirectIblLightSample(GIIntegrationContext* __restrict context, LightDirectSample& sample)
    {
        // -- choose direction to sample the ibl
        float r0 = context->sampler.UniformFloat();
        float r1 = context->sampler.UniformFloat();

        uint x;
        uint y;
        float dirPhi;
        float dirTheta;
        // -- Importance sample the ibl. Note that we're cheating and treating the sample pdf as an area measure
        // -- even though it's a solid angle measure.
        Ibl(&context->sceneData->ibl->densityfunctions, r0, r1, dirTheta, dirPhi, x, y, sample.directionPdfA);
        float3 toIbl = Math::SphericalToCartesian(dirTheta, dirPhi);
        float3 radiance = SkyIntensityScale_ * SampleIbl(context->sceneData->ibl, x, y);

        float sceneBoundingRadius = context->sceneData->scene->data->boundingSphere.w;

        sample.distance = 1e36f;
        sample.direction = toIbl;
        sample.radiance = radiance;
        sample.emissionPdfW = sample.directionPdfA * ConcentricDiscPdf() * (1.0f / (sceneBoundingRadius*sceneBoundingRadius));
        sample.cosThetaLight = 1.0f; // -- not used for ibl light source
    }

    //=============================================================================================================================
    float3 IblCalculateRadiance(GIIntegrationContext* __restrict context, float3 direction, float& directPdfA, float& emissionPdfW)
    {
        float iblPdfA;
        float3 radiance = SkyIntensityScale_ * SampleIbl(context->sceneData->ibl, direction, iblPdfA);

        float sceneBoundingRadius = context->sceneData->scene->data->boundingSphere.w;
        float pdfPosition = ConcentricDiscPdf() * (1.0f / (sceneBoundingRadius * sceneBoundingRadius));

        directPdfA = iblPdfA;
        emissionPdfW = pdfPosition * iblPdfA;
        return radiance;
    }

    //=============================================================================================================================
    void BackgroundLightSample(GIIntegrationContext* __restrict context, LightDirectSample& sample)
    {
        // -- choose direction to sample the ibl
        float r0 = context->sampler.UniformFloat();
        float r1 = context->sampler.UniformFloat();

        float u = 2.0f * r1 - 1.0f;
        float norm = Math::Sqrtf(Max(0.0f, 1.0f - u * u));
        float theta = Math::TwoPi_ * r0;

        float3 direction = float3(norm * Math::Cosf(theta), u, norm * Math::Sinf(theta));

        sample.distance = 1e36f;
        sample.direction = direction;
        sample.radiance = context->sceneData->scene->data->backgroundIntensity;
        sample.emissionPdfW = Math::Inv4Pi_;
        sample.directionPdfA = Math::Inv4Pi_;

        sample.cosThetaLight = 1.0f; // -- not used for background light source
    }

    //=============================================================================================================================
    void NextEventEstimation(GIIntegrationContext* context, LightDirectSample& sample)
    {
        SceneContext* sceneData = context->sceneData;

        if(sceneData->ibl) {
            DirectIblLightSample(context, sample);
            return;
        }

        BackgroundLightSample(context, sample);
    }

    //=============================================================================================================================
    float BackgroundLightingPdf(GIIntegrationContext* context, float3 wi)
    {
        if(context->sceneData->ibl) {
            return SampleIBlPdf(context->sceneData->ibl, wi);
        }
        else {
            return Math::Inv4Pi_;
        }
    }

    //=============================================================================================================================
    float3 SampleBackgroundLight(GIIntegrationContext* context, float3 wi)
    {
        if(context->sceneData->ibl) {
            float pdf;
            return SkyIntensityScale_ * SampleIbl(context->sceneData->ibl, wi, pdf);
        }
        else {
            return context->sceneData->scene->data->backgroundIntensity;
        }
    }
}