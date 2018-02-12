
//==============================================================================
// Joe Schutte
//==============================================================================

#include "PathTracerShading.h"
#include "Brdf.h"

#include <SceneLib/SceneResource.h>
#include <SceneLib/ImageBasedLightResource.h>
#include <UtilityLib/Color.h>
#include <MathLib/FloatFuncs.h>
#include <MathLib/FloatStructs.h>
#include <MathLib/Trigonometric.h>
#include <MathLib/ImportanceSampling.h>
#include <MathLib/Random.h>
#include <MathLib/Projection.h>
#include <MathLib/Quaternion.h>
#include <ContainersLib/Rect.h>
#include <SystemLib/MemoryAllocation.h>
#include <SystemLib/BasicTypes.h>

#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>


namespace Shooty
{
    //==============================================================================
    static bool OcclusionRay(RTCScene& rtcScene, float3 orig, float3 direction, float znear, float zfar)
    {
        RTCIntersectContext context;
        rtcInitIntersectContext(&context);

        __declspec(align(16)) RTCRay ray;
        ray.org_x = orig.x;
        ray.org_y = orig.y;
        ray.org_z = orig.z;
        ray.dir_x = direction.x;
        ray.dir_y = direction.y;
        ray.dir_z = direction.z;
        ray.tnear = znear;
        ray.tfar = zfar;

        rtcOccluded1(rtcScene, &context, &ray);

        // -- ray.tfar == -inf when hit occurs
        return (ray.tfar >= 0.0f);
    }

    //------------------------------------------------------------------------------
    static float Attenuation(float distance)
    {
        const float lightRange = 1000.0f;
        const float virtualRadius = 1.0f; // fake light is fake

        float linear = 2.0f / virtualRadius;
        float quadratic = 1.0f / (virtualRadius * virtualRadius);
        float shift = 1.0f / (1.0f + lightRange * (linear + lightRange * quadratic));

        return Saturate((1.0f / (1.0f + distance * (linear + distance * quadratic))) - shift);
    }

    //==============================================================================
    float3 ImportanceSampleIbl(RTCScene& rtcScene, ImageBasedLightResourceData* ibl, Random::MersenneTwister* twister, float3 p, float3 n, float3 v, float3 albedo, float3 reflectance, float roughness)
    {
        const uint sampleCount = 1024;

        float3 lighting = float3::Zero_;

        for(uint scan = 0; scan < sampleCount; ++scan) {
            float r0 = Random::MersenneTwisterFloat(twister);
            float r1 = Random::MersenneTwisterFloat(twister);

            float theta;
            float phi;
            uint x;
            uint y;
            float weight;
            ImportanceSampling::Ibl(&ibl->densityfunctions, r0, r1, theta, phi, x, y, weight);

            float3 wi = Math::SphericalToCartesian(theta, phi);
            if(Dot(wi, n) <= 0.0f)
                continue;

            if(OcclusionRay(rtcScene, p, wi, 0.001f, FloatMax_)) {
                float3 sample = ibl->hdrData[y * ibl->densityfunctions.width + x];
                float3 irradiance = GgxBrdf(n, wi, v, albedo, reflectance, roughness);

                lighting += sample * irradiance * weight;
            }
        }

        return lighting * (1.0f / sampleCount);
    }

    //==============================================================================
    float3 ImportanceSampleGgx(ImageBasedLightResourceData* ibl, Random::MersenneTwister* twister, float3 n, float3 v, float3 albedo, float3 reflectance, float roughness)
    {
        const uint sampleCount = 1024;

        float3 lighting = float3::Zero_;

        float3 axis = Normalize(Cross(float3::YAxis_, n));
        float radians = Math::Acosf(Dot(float3::YAxis_, n));

        float4 q = Math::Quaternion::Create(radians, axis);

        for(uint scan = 0; scan < sampleCount; ++scan) {
            float r0 = Random::MersenneTwisterFloat(twister);
            float r1 = Random::MersenneTwisterFloat(twister);

            float theta;
            float phi;
            float weight;
            ImportanceSampling::Ggx(roughness, r0, r1, theta, phi, weight);

            float3 wi = Math::SphericalToCartesian(theta, phi);
            wi = Math::Quaternion::Rotate(q, wi);
            if(Dot(wi, n) <= 0.0f)
                continue;

            Math::NormalizedCartesianToSpherical(wi, theta, phi);

            uint x = (uint)((phi / Math::TwoPi_) * ibl->densityfunctions.width - 0.5f);
            uint y = (uint)((theta / Math::Pi_) * ibl->densityfunctions.height - 0.5f);
            Assert_(x < ibl->densityfunctions.width);
            Assert_(y < ibl->densityfunctions.height);

            float3 sample = ibl->hdrData[y * ibl->densityfunctions.width + x];
            float3 irradiance = GgxBrdf(n, wi, v, albedo, reflectance, roughness);

            lighting += sample * irradiance * weight;
        }

        return lighting * (1.0f / sampleCount);
    }
}