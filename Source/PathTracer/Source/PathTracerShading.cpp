
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
#include <SystemLib/MinMax.h>

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
    float3 SampleIbl(ImageBasedLightResourceData* ibl, float3 wi)
    {
        float theta;
        float phi;
        Math::NormalizedCartesianToSpherical(wi, theta, phi);

        uint x = (uint)((phi / Math::TwoPi_) * ibl->densityfunctions.width - 0.5f);
        uint y = (uint)((theta / Math::Pi_) * ibl->densityfunctions.height - 0.5f);
        Assert_(x < ibl->densityfunctions.width);
        Assert_(y < ibl->densityfunctions.height);
        return ibl->hdrData[y * ibl->densityfunctions.width + x];
    }

    //==============================================================================
    void ImportanceSampleIbl(RTCScene& rtcScene, ImageBasedLightResourceData* ibl, Random::MersenneTwister* twister, float3 p, float3 n, float3 v, Material* material,
                             float3& wi, float3& reflectance)
    {
        reflectance = float3::Zero_;
        const float bias = 0.0001f;

        float r0 = Random::MersenneTwisterFloat(twister);
        float r1 = Random::MersenneTwisterFloat(twister);

        float theta;
        float phi;
        uint x;
        uint y;
        float weight;
        ImportanceSampling::Ibl(&ibl->densityfunctions, r0, r1, theta, phi, x, y, weight);

        wi = Math::SphericalToCartesian(theta, phi);
        if(Dot(wi, n) > 0.0f) {
            if(OcclusionRay(rtcScene, p + bias * n, wi, 0, FloatMax_)) {
                float3 sample = ibl->hdrData[y * ibl->densityfunctions.width + x];
                reflectance = sample * weight * GgxBrdf(n, wi, v, material->albedo, material->specularColor, material->roughness);
            }
        }
    }

    //==============================================================================
    static float3 OrientToNormal(float3 n, float3 l)
    {
        float3 axis = Cross(float3::YAxis_, n);
        float len = Length(axis);

        if(len > MinFloatEpsilon_) {
            axis = (1.0f / len) * axis;
            float radians = Math::Atan2f(len, n.y);

            float4 q = Math::Quaternion::Create(radians, axis);
            return Math::Quaternion::Rotate(q, l);
        }
        else if(n.y > 0.0f) {
            return l;
        }
        else {
            return -l;
        }
    }

    //==============================================================================
    void ImportanceSampleGgx(Random::MersenneTwister* twister, float3 n, float3 v, Material* material, float3& wi, float3& reflectance)
    {
        float r0 = Random::MersenneTwisterFloat(twister);
        float r1 = Random::MersenneTwisterFloat(twister);

        float theta;
        float phi;
        float weight;
        ImportanceSampling::Ggx(material->roughness, r0, r1, theta, phi, weight);

        wi = OrientToNormal(n, Math::SphericalToCartesian(theta, phi));
        if(Dot(wi, n) > 0.0f) {
            reflectance = weight * GgxBrdf(n, wi, v, material->albedo, material->specularColor, material->roughness);
        }
        else {
            reflectance = float3::Zero_;
        }
    }
}