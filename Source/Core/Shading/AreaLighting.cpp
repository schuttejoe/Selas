
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
#include "MathLib/GeometryIntersection.h"
#include "MathLib/Random.h"
#include "MathLib/Projection.h"
#include "MathLib/Quaternion.h"
#include "MathLib/GeometryIntersection.h"
#include "SystemLib/MinMax.h"

namespace Selas
{
    //=============================================================================================================================
    float QuadLightAreaPdf(const SceneLight& light, const float3& position, const float3& wi)
    {
        float3 eX = light.x;
        float3 eZ = light.z;
        float3 v00 = light.position - 0.5f * eX - 0.5f * eZ;
        float3 v01 = light.position - 0.5f * eX + 0.5f * eZ;
        float3 v10 = light.position + 0.5f * eX - 0.5f * eZ;
        float3 v11 = light.position + 0.5f * eX + 0.5f * eZ;

        if(Intersection::RayQuad(position, wi, v00, v10, v01, v11)) {
            float eXLength = Length(eX);
            float eZLength = Length(eZ);

            float pdf = 1.0f / (eXLength * eZLength);
            return pdf;
        }

        return 0.0f;
    }

    //=============================================================================================================================
    float QuadLightSolidAnglePdf(const SceneLight& light, const float3& position, const float3& wi)
    {
        float3 eX = light.x;
        float3 eZ = light.z;
        float3 v00 = light.position - 0.5f * eX - 0.5f * eZ;
        float3 v01 = light.position - 0.5f * eX + 0.5f * eZ;
        float3 v10 = light.position + 0.5f * eX - 0.5f * eZ;
        float3 v11 = light.position + 0.5f * eX + 0.5f * eZ;

        if(Intersection::RayQuad(position, wi, v00, v10, v01, v11)) {
            RectangleLightSampler quadsampler;
            InitializeRectangleLightSampler(v00, eX, eZ, position, quadsampler);

            return 1.0f / quadsampler.S;
        }

        return 0.0f;
    }

    //=============================================================================================================================
    void IntegrateRectangleLightWithArea(GIIntegratorContext* context, const float3& position, const float3& normal,
                                         const SceneLight& light, LightDirectSample& sample)
    {
        float3 eX = light.x;
        float3 eZ = light.z;
        float3 s = light.position - 0.5f * eX - 0.5f * eZ;

        float eXLength = Length(eX);
        float eZLength = Length(eZ);

        float3 lightFacing = light.direction;

        float3 Lo = float3::Zero_;

        float pdf = 1.0f / (eXLength * eZLength);

        float u = context->sampler.UniformFloat();
        float v = context->sampler.UniformFloat();

        float3 lp = s + u * eX + v * eZ;

        float3 ul = lp - position;
        float distSquared = LengthSquared(ul);
        float dist = Math::Sqrtf(distSquared);
        float3 l = (1.0f / dist) * ul;

        float dotNL = Saturate(Dot(normal, l));
        float dotSL = Saturate(Dot(lightFacing, -l));

        if(dotNL > 0.0f && dotSL > 0.0f) {
            sample.direction = l;
            sample.distance = dist;
            sample.radiance = light.radiance * dotSL / distSquared;
            sample.pdfW = pdf;
        }
        else {
            sample.direction = float3::Zero_;
            sample.distance = 0.0f;
            sample.radiance = float3::Zero_;
            sample.pdfW = 1.0f;
        }
    }

    //=============================================================================================================================
    static void SampleRectangleLightSolidAngle(GIIntegratorContext* context, const float3& position, const float3& normal,
                                               const SceneLight& light, LightDirectSample& sample)
    {
        float3 eX = light.x;
        float3 eZ = light.z;
        float3 s = light.position - 0.5f * eX - 0.5f * eZ;

        RectangleLightSampler quadsampler;
        InitializeRectangleLightSampler(s, eX, eZ, position, quadsampler);

        float3 lightFacing = quadsampler.z;

        float3 Lo = float3::Zero_;

        float pdf = 1.0f / quadsampler.S;

        float u = context->sampler.UniformFloat();
        float v = context->sampler.UniformFloat();

        float3 lp = SampleRectangleLight(quadsampler, u, v);

        float3 ul = lp - position;
        float distSquared = LengthSquared(ul);
        float dist = Math::Sqrtf(distSquared);
        float3 l = (1.0f / dist) * ul;
        
        float areaMeasure = Dot(light.direction, -l) / distSquared;

        if(areaMeasure > 0) {
            sample.direction = l;
            sample.distance = dist;
            sample.radiance = light.radiance * areaMeasure;
            sample.pdfW = pdf * areaMeasure;
        }
        else {
            sample.direction = float3::Zero_;
            sample.distance = 0.0f;
            sample.radiance = float3::Zero_;
            sample.pdfW = 1.0f;
        }
    }

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
    void EmitIblLightSample(GIIntegratorContext* __restrict context, LightEmissionSample& sample)
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

        Assert_(context->scene->iblResource != nullptr);
        ImageBasedLightResourceData* iblData = context->scene->iblResource->data;

        // -- Importance sample the ibl. Note that we're cheating and treating the sample pdf as an area measure
        // -- even though it's a solid angle measure.
        Ibl(iblData, r0, r1, dirTheta, dirPhi, x, y, sample.directionPdfA);
        float3 toIbl = Math::SphericalToCartesian(dirTheta, dirPhi);
        float3 radiance = SampleIbl(iblData, x, y);

        float3 dX, dZ;
        MakeOrthogonalCoordinateSystem(toIbl, &dX, &dZ);

        float sceneBoundingRadius = context->scene->boundingSphere.w;
        float3 sceneCenter = context->scene->boundingSphere.XYZ();

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
    void DirectIblLightSample(GIIntegratorContext* __restrict context, LightDirectSample& sample)
    {
        // -- choose direction to sample the ibl
        float r0 = context->sampler.UniformFloat();
        float r1 = context->sampler.UniformFloat();

        Assert_(context->scene->iblResource != nullptr);
        ImageBasedLightResourceData* iblData = context->scene->iblResource->data;

        uint x;
        uint y;
        float dirPhi;
        float dirTheta;

        Ibl(iblData, r0, r1, dirTheta, dirPhi, x, y, sample.pdfW);
        float3 toIbl = Math::SphericalToCartesian(dirTheta, dirPhi);
        float3 radiance = SampleIbl(iblData, x, y);

        sample.distance = 1e36f;
        sample.direction = toIbl;
        sample.radiance = radiance;
    }

    //=============================================================================================================================
    float3 IblCalculateRadiance(GIIntegratorContext* __restrict context, float3 direction, float& directPdfA, float& emissionPdfW)
    {
        Assert_(context->scene->iblResource != nullptr);
        ImageBasedLightResourceData* iblData = context->scene->iblResource->data;

        float iblPdfA;
        float3 radiance = SampleIbl(iblData, direction, iblPdfA);

        float sceneBoundingRadius = context->scene->boundingSphere.w;
        float pdfPosition = ConcentricDiscPdf() * (1.0f / (sceneBoundingRadius * sceneBoundingRadius));

        directPdfA = iblPdfA;
        emissionPdfW = pdfPosition * iblPdfA;
        return radiance;
    }

    //=============================================================================================================================
    static void BackgroundLightSample(GIIntegratorContext* __restrict context, LightDirectSample& sample)
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
        sample.radiance = context->scene->data->backgroundIntensity.XYZ();
        sample.pdfW = 1.0f;
    }

    //=============================================================================================================================
    void NextEventEstimation(GIIntegratorContext* context, uint lightSetIndex, const float3& position, const float3& normal,
                             LightDirectSample& sample)
    {
        const SceneLightSet& lightSet = context->scene->data->lightsets[lightSetIndex];

        uint lightCount = lightSet.lights.Count();
        if(lightCount == 0) {
            return;
        }

        float perLightProb = 1.0f / lightCount;

        float p0 = context->sampler.UniformFloat();
        uint lightIndex = (uint)(p0 * lightCount);
        SampleRectangleLightSolidAngle(context, position, normal, lightSet.lights[lightIndex], sample);
        sample.pdfW *= perLightProb;
        sample.index = (uint32)lightIndex;
    }

    //=============================================================================================================================
    float LightingPdf(GIIntegratorContext* context, uint lightSetIndex, const LightDirectSample& light,
                      const float3& position, const float3& wi)
    {
        const SceneLightSet& lightSet = context->scene->data->lightsets[lightSetIndex];

        uint lightCount = lightSet.lights.Count();
        if(lightCount == 0) {
            return 0.0f;
        }

        float perLightProb = 1.0f / lightCount;
        return QuadLightSolidAnglePdf(lightSet.lights[light.index], position, wi) * perLightProb;
    }

    //=============================================================================================================================
    void SampleBackground(GIIntegratorContext* context, LightDirectSample& sample)
    {
        if(context->scene->iblResource) {
            return DirectIblLightSample(context, sample);
        }
        else {
            return BackgroundLightSample(context, sample);
        }
    }

    //=============================================================================================================================
    float BackgroundLightingPdf(GIIntegratorContext* context, float3 wi)
    {
        if(context->scene->iblResource) {
            return SampleIBlPdf(context->scene->iblResource->data, wi);
        }
        else {
            return Math::Inv4Pi_;
        }
    }

    //=============================================================================================================================
    float3 EvaluateBackground(GIIntegratorContext* context, float3 wi)
    {
        if(context->scene->iblResource) {
            float pdf;
            return SampleIbl(context->scene->iblResource->data, wi, pdf);
        }
        else {
            return context->scene->data->backgroundIntensity.XYZ();
        }
    }

    //=============================================================================================================================
    float3 EvaluateBackgroundMiss(GIIntegratorContext* context, float3 wi)
    {
        if(context->scene->iblResource) {
            float pdf;
            return SampleIblMiss(context->scene->iblResource->data, wi, pdf);
        }
        else {
            return context->scene->data->backgroundIntensity.XYZ();
        }
    }
}