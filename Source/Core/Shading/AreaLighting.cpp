
//==============================================================================
// Joe Schutte
//==============================================================================

#include <Shading/AreaLighting.h>
#include <Shading/SurfaceParameters.h>
#include <Shading/Shading.h>
#include <Shading/IntegratorContexts.h>

#include <SceneLib/SceneResource.h>
#include <GeometryLib/RectangulerLight.h>
#include <GeometryLib/CoordinateSystem.h>
#include <MathLib/FloatFuncs.h>
#include <MathLib/Trigonometric.h>
#include <MathLib/ImportanceSampling.h>
#include <MathLib/Random.h>
#include <MathLib/Projection.h>
#include <MathLib/Quaternion.h>
#include <MathLib/GeometryIntersection.h>

#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>

namespace Shooty
{
    #define MaxBounceCount_  10
    
    //==============================================================================
    bool OcclusionRay(RTCScene& rtcScene, const SurfaceParameters& surface, float3 direction, float distance)
    {
        // -- Why does this need to be so "large" to avoid artifacts when lighting from a point light?
        float3 origin = OffsetRayOrigin(surface, direction, 64.0f);

        RTCIntersectContext context;
        rtcInitIntersectContext(&context);

        __declspec(align(16)) RTCRay ray;
        ray.org_x = origin.x;
        ray.org_y = origin.y;
        ray.org_z = origin.z;
        ray.dir_x = direction.x;
        ray.dir_y = direction.y;
        ray.dir_z = direction.z;
        ray.tnear = surface.error;
        ray.tfar  = distance;

        rtcOccluded1(rtcScene, &context, &ray);

        // -- ray.tfar == -inf when hit occurs
        return (ray.tfar >= 0.0f);
    }

    //==============================================================================
    float3 IntegrateRectangleLightWithArea(RTCScene& rtcScene, Random::MersenneTwister* twister, const SurfaceParameters& surface, RectangularAreaLight light, uint sampleCount)
    {
        float3 eX = light.eX;
        float3 eZ = light.eZ;
        float3 s = light.corner;

        float eXLength = Length(eX);
        float eZLength = Length(eZ);

        float3 lightFacing = Normalize(Cross(eX, eZ));

        if(Dot(-lightFacing, surface.perturbedNormal) <= 0.0f) {
            return float3::Zero_;
        }

        float3 Lo = float3::Zero_;

        float pdf = 1.0f / (eXLength * eZLength);

        for(uint scan = 0; scan < sampleCount; ++scan) {
            float u = Random::MersenneTwisterFloat(twister);
            float v = Random::MersenneTwisterFloat(twister);

            float3 position = s + u * eX + v * eZ;

            float3 ul = position - surface.position;
            float distSquared = LengthSquared(ul);
            float dist = Math::Sqrtf(distSquared);
            float3 l = (1.0f / dist) * ul;

            float dotNL = Saturate(Dot(surface.perturbedNormal, l));
            float dotSL = Saturate(Dot(lightFacing, -l));

            if(dotNL > 0.0f && dotSL > 0.0f && OcclusionRay(rtcScene, surface, l, dist)) {
                Lo += (dotNL * dotSL / distSquared) * light.intensity;
            }
        }

        return Lo * (1.0f / (pdf * sampleCount));
    }

    //==============================================================================
    float3 IntegrateRectangleLightWithSolidAngle(RTCScene& rtcScene, Random::MersenneTwister* twister, const SurfaceParameters& surface, RectangularAreaLight light, uint sampleCount)
    {
        float3 eX = light.eX;
        float3 eZ = light.eZ;
        float3 s  = light.corner;

        RectangleLightSampler sampler;
        InitializeRectangleLightSampler(s, eX, eZ, surface.position, sampler);

        float3 lightFacing = sampler.z;

        float3 Lo = float3::Zero_;

        float pdf = 1.0f / sampler.S;

        for(uint scan = 0; scan < sampleCount; ++scan) {
            float u = Random::MersenneTwisterFloat(twister);
            float v = Random::MersenneTwisterFloat(twister);

            float3 position = SampleRectangleLight(sampler, u, v);

            float3 ul = position - surface.position;
            float distSquared = LengthSquared(ul);
            float dist = Math::Sqrtf(distSquared);
            float3 l = (1.0f / dist) * ul;

            float dotNL = Saturate(Dot(surface.perturbedNormal, l));
            float dotSL = Saturate(Dot(lightFacing, -l));

            if(dotNL > 0.0f && dotSL > 0.0f && OcclusionRay(rtcScene, surface, l, dist)) {
                // -- the dist^2 and Dot(w', n') terms from the pdf and the area form of the rendering equation cancel out
                // -- the pdf is constant so that is applied below.
                Lo +=  dotNL * light.intensity;
            }
        }

        return Lo * (1.0f / (pdf * sampleCount));
    }

    //==============================================================================
    float3 IntegrateSphereLightWithAreaSampling(RTCScene& rtcScene, Random::MersenneTwister* twister, const SurfaceParameters& surface, SphericalAreaLight light, uint lightSampleCount)
    {
        float3 L = light.intensity;
        float3 c = light.center;
        float  r = light.radius;

        float3 Lo = float3::Zero_;

        float pdf = 1.0f / (4.0f * Math::Pi_ * r * r);

        for(uint scan = 0; scan < lightSampleCount; ++scan) {
            float r0 = Random::MersenneTwisterFloat(twister);
            float r1 = Random::MersenneTwisterFloat(twister);

            float theta = Math::Acosf(1 - 2.0f * r0);
            float phi   = Math::TwoPi_ * r1;

            float3 sn = Math::SphericalToCartesian(theta, phi);
            float3 xp = c + r * sn;

            float3 w = xp - surface.position;
            float distSquared = Dot(w, w);
            float dist = Math::Sqrtf(distSquared);
            w = (1.0f / dist) * w;

            float dotSL = Dot(sn, -w);
            float dotNL = Dot(surface.perturbedNormal, w);

            if(dotSL > 0.0f && dotNL > 0.0f && OcclusionRay(rtcScene, surface, w, dist)) {
                float bsdf = dotNL;
                Lo += L * bsdf * dotSL * (1.0f / distSquared);
            }
        }

        return Lo * (1.0f / (lightSampleCount * pdf));
    }

    //==============================================================================
    float3 IntegrateSphereLightWithSolidAngleSampling(RTCScene& rtcScene, Random::MersenneTwister* twister, const SurfaceParameters& surface, float3 view, SphericalAreaLight light, uint lightSampleCount)
    {
        float3 L = light.intensity;
        float3 c = light.center;
        float  r = light.radius;

        float3 w = c - surface.position;
        float distanceToCenter = Length(w);
        w = w * (1.0f / distanceToCenter);

        float q = Math::Sqrtf(1.0f - (r / distanceToCenter) * (r / distanceToCenter));

        float3 v, u;
        MakeOrthogonalCoordinateSystem(w, &v, &u);        

        float3x3 toWorld = MakeFloat3x3(u, w, v);
        
        float3 Lo = float3::Zero_;

        for(uint scan = 0; scan < lightSampleCount; ++scan) {
            float r0 = Random::MersenneTwisterFloat(twister);
            float r1 = Random::MersenneTwisterFloat(twister);

            float theta = Math::Acosf(1 - r0 + r0 * q);
            float phi   = Math::TwoPi_ * r1;

            float3 nwp = MatrixMultiply(Math::SphericalToCartesian(theta, phi), toWorld);
            float3 wp = -nwp;

            float3 xp;
            Intersection::RaySphereNearest(surface.position, nwp, c, r, xp);

            float distSquared = LengthSquared(xp - surface.position);
            float dist = Math::Sqrtf(distSquared);

            float dotNL = Saturate(Dot(nwp, surface.perturbedNormal));
            if(dotNL > 0.0f && OcclusionRay(rtcScene, surface, nwp, dist)) {
                // -- the dist^2 and Dot(w', n') terms from the pdf and the area form of the rendering equation cancel out
                float pdf_xp = 1.0f / (Math::TwoPi_ * (1.0f - q));
                float bsdfPdf;
                float3 bsdf = EvaluateBsdf(surface, view, nwp, bsdfPdf);
                Lo += bsdf * (1.0f / pdf_xp) * L;
            }
        }

        return Lo * (1.0f / lightSampleCount);
    }

    //==============================================================================
    void GenerateIblLightSample(KernelContext* __restrict context, LightSample& sample)
    {
        // -- http://www.iliyan.com/publications/ImplementingVCM/ImplementingVCM_TechRep2012_rev2.pdf
        // -- see section 5.1 of ^ to understand the position and emission pdf calculation

        // -- choose direction to sample the ibl
        float r0 = Random::MersenneTwisterFloat(context->twister);
        float r1 = Random::MersenneTwisterFloat(context->twister);

        uint x;
        uint y;
        float phi;
        float theta;
        // -- Importance sample the ibl. Note that we're cheating and treating the sample pdf as an area measure
        // -- even though it's a solid angle measure.
        Ibl(&context->sceneData->ibl->densityfunctions, r0, r1, theta, phi, x, y, sample.directionPdfA);

        sample.direction = -Math::SphericalToCartesian(theta, phi);
        sample.radiance = SampleIbl(context->sceneData->ibl, x, y);

        float sceneBoundingRadius = context->sceneData->scene->data->boundingSphere.w;
        sample.position = context->sceneData->scene->data->boundingSphere.XYZ() + sceneBoundingRadius * -sample.direction;

        sample.emissionPdfW = sample.directionPdfA * context->sceneData->invSquareBoundingRadius;
        sample.cosThetaLight = 1.0f; // -- not used
    }
}