
//==============================================================================
// Joe Schutte
//==============================================================================

#include "PathTracerShading.h"
#include "SurfaceParameters.h"
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

    ////==============================================================================
    //void ImportanceSampleIbl(RTCScene& rtcScene, ImageBasedLightResourceData* ibl, Random::MersenneTwister* twister, float3 p, float3 n, float3 v, Material* material,
    //                         float3& wi, float3& reflectance)
    //{
    //    reflectance = float3::Zero_;
    //    const float bias = 0.0001f;

    //    float r0 = Random::MersenneTwisterFloat(twister);
    //    float r1 = Random::MersenneTwisterFloat(twister);

    //    float theta;
    //    float phi;
    //    uint x;
    //    uint y;
    //    float iblPdf;
    //    ImportanceSampling::Ibl(&ibl->densityfunctions, r0, r1, theta, phi, x, y, iblPdf);

    //    wi = Math::SphericalToCartesian(theta, phi);
    //    if(Dot(wi, n) > 0.0f) {
    //        if(OcclusionRay(rtcScene, p + bias * n, wi, 0, FloatMax_)) {
    //            float3 sample = ibl->hdrData[y * ibl->densityfunctions.width + x];
    //            reflectance = sample * GgxBrdf(n, wi, v, material->albedo, material->specularColor, material->roughness) * (1.0f / iblPdf);
    //        }
    //    }
    //}

    //==============================================================================
    static float4 ToTangentSpaceQuaternion(float3 n)
    {
        float3 axis = Cross(n, float3::YAxis_);
        float len = Length(axis);

        if(len > MinFloatEpsilon_) {
            axis = (1.0f / len) * axis;
            float radians = Math::Atan2f(len, n.y);

            return Math::Quaternion::Create(radians, axis);
        }
        else if(n.y > 0.0f) {
            return Math::Quaternion::Identity();
        }
        else {
            return Math::Quaternion::Create(180.0f * Math::DegreesToRadians_, float3::XAxis_);
        }
    }

    // ================================================================================================
    // Fresnel
    // ================================================================================================
    static float3 SchlickFresnel(float3 r0, float radians)
    {
        float exponential = Math::Powf(1.0f - radians, 5.0f);
        return r0 + float3(1.0f - r0.x, 1.0f - r0.y, 1.0f - r0.z) * exponential;
    }

    static float SchlickFresnel(float u)
    {
        float m = Saturate(1.0f - u);
        float m2 = m * m;
        return m * m2 * m2;
    }

    //==============================================================================
    static float BsdfNDot(float3 x)
    {
        return x.y;
    }

    //==============================================================================
    static float SmithGGXMasking(float3 wi, float3 wo, float3 wm, float a2)
    {
        float dotNL = BsdfNDot(wi);
        float dotNV = BsdfNDot(wo);
        float denomC = Math::Sqrtf(a2 + (1.0f - a2) * dotNV * dotNV) + dotNV;

        return 2.0f * dotNV / denomC;
    }

    //==============================================================================
    static float SmithGGXMaskingShading(float3 wi, float3 wo, float3 wm, float a2)
    {
        // non height-correlated masking function
        // https://twvideo01.ubm-us.net/o1/vault/gdc2017/Presentations/Hammon_Earl_PBR_Diffuse_Lighting.pdf
        float dotNL = BsdfNDot(wi);
        float dotNV = BsdfNDot(wo);

        float denomA = dotNV * Math::Sqrtf(a2 + (1.0f - a2) * dotNL * dotNL);
        float denomB = dotNL * Math::Sqrtf(a2 + (1.0f - a2) * dotNV * dotNV);

        return 2.0f * dotNL * dotNV / (denomA + denomB);
    }

    //==============================================================================
    void ImportanceSampleGgxD(Random::MersenneTwister* twister, float3 wg, float3 v, Material* material, float3& wi, float3& reflectance)
    {
        // -- build tangent space transforms // -- JSTODO - get tangent space from mesh
        float4 toLocal = ToTangentSpaceQuaternion(wg);
        float4 toWorld = Math::Quaternion::Invert(toLocal);

        float3 wo = Math::Quaternion::Rotate(toLocal, v);

        float theta;
        float phi;
        float r0 = Random::MersenneTwisterFloat(twister);
        float r1 = Random::MersenneTwisterFloat(twister);
        ImportanceSampling::GgxD(material->roughness, r0, r1, theta, phi);

        float3 wm = Math::SphericalToCartesian(theta, phi);
        wi = Reflect(wm, wo);

        if(BsdfNDot(wi) > 0.0f && Dot(wi, wm) > 0.0f) {
            float a = material->roughness;
            float a2 = a * a;

            float3 F = SchlickFresnel(material->specularColor, Dot(wi, wm));
            float G = SmithGGXMaskingShading(wi, wo, wm, a2);
            float weight = Math::Absf(Dot(wo, wm)) / (BsdfNDot(wo) * BsdfNDot(wm));

            reflectance = F * G * weight;
            wi = Math::Quaternion::Rotate(toWorld, wi);    
        }
        else {
            reflectance = float3::Zero_;
        }
    }

    //==============================================================================
    // JSTODO - This is temp. Get tangent space from mesh.
    static float3 TransformToWorld(float x, float y, float z, float3 normal)
    {
        // Find an axis that is not parallel to normal
        float3 majorAxis;
        if(Math::Absf(normal.x) < 0.57735026919f) {
            majorAxis = float3(1, 0, 0);
        }
        else if(Math::Absf(normal.y) < 0.57735026919f) {
            majorAxis = float3(0, 1, 0);
        }
        else {
            majorAxis = float3(0, 0, 1);
        }

        // Use majorAxis to create a coordinate system relative to world space
        float3 u = Normalize(Cross(normal, majorAxis));
        float3 v = Cross(normal, u);
        float3 w = normal;

        // Transform from local coordinates to world coordinates
        return u * x + w * y + v * z;
    }

    //==============================================================================
    //void ImportanceSampleLambert(Random::MersenneTwister* twister, float3 n, float3 v, Material* material, float3& wi, float3& reflectance)
    //{
    //    float r0 = Random::MersenneTwisterFloat(twister);
    //    float r1 = Random::MersenneTwisterFloat(twister);

    //    float r     = Math::Sqrtf(r0);
    //    float theta = 2.0f * Math::Pi_ * r1;

    //    float x     = r * Math::Cosf(theta);
    //    float y     = Math::Sqrtf(Max<float>(0.0f, 1.0f - r0));
    //    float z     = r * Math::Sinf(theta);

    //    wi = TransformToWorld(x, y, z, n);

    //    float dotNL = Dot(wi, n);
    //    if(dotNL > 0.0f) {
    //        reflectance = material->albedo;
    //    }
    //    else {
    //        reflectance = float3::Zero_;
    //    }
    //}

    //==============================================================================
    void ImportanceSampleGgxVdn(Random::MersenneTwister* twister, float3 wg, float3 v, Material* material, float3& wi, float3& reflectance)
    {
        float a = material->roughness;
        float a2 = a * a;

        // -- build tangent space transforms // -- JSTODO - get tangent space from mesh
        float4 toLocal = ToTangentSpaceQuaternion(wg);
        float4 toWorld = Math::Quaternion::Invert(toLocal);

        float3 wo = Math::Quaternion::Rotate(toLocal, v);

        float r0 = Random::MersenneTwisterFloat(twister);
        float r1 = Random::MersenneTwisterFloat(twister);
        float3 wm = ImportanceSampling::GgxVndf(wo, material->roughness, r0, r1);

        float3 localWi = Reflect(wm, wo);

        if(BsdfNDot(localWi) > 0.0f) {
            wi = Math::Quaternion::Rotate(toWorld, localWi);

            float3 F = SchlickFresnel(material->specularColor, Dot(localWi, wm));
            float G1 = SmithGGXMasking(localWi, wo, wm, a2);
            float G2 = SmithGGXMaskingShading(localWi, wo, wm, a2);

            reflectance = F * (G2 / G1);
            
        }
        else {
            reflectance = float3::Zero_;
        }
    }

    //==============================================================================
    void ImportanceSampleDisneyBrdf(Random::MersenneTwister* twister, const SurfaceParameters& surface, float3 v, float3& wi, float3& reflectance)
    {
        float3 wg = surface.normal;

        // -- build tangent space transforms // -- JSTODO - get tangent space from mesh
        float4 toLocal = ToTangentSpaceQuaternion(wg);
        float4 toWorld = Math::Quaternion::Invert(toLocal);

        float a = surface.roughness;
        float a2 = a * a;

        float3 wo = Math::Quaternion::Rotate(toLocal, v);

        float r0 = Random::MersenneTwisterFloat(twister);
        float r1 = Random::MersenneTwisterFloat(twister);
        float3 wm = ImportanceSampling::GgxVndf(wo, surface.roughness, r0, r1);

        float3 localWi = Reflect(wm, wo);

        reflectance = float3::Zero_;

        if(BsdfNDot(localWi) > 0.0f) {
            float3 F = SchlickFresnel(surface.specularColor, Dot(localWi, wm));
            // -- JSTODO - Wait. I'm using wo.wm and wi.wm here while disney.brdf is using n.l and n.v. Which is correct?
            float G1 = SmithGGXMasking(localWi, wo, wm, a2);
            float G2 = SmithGGXMaskingShading(localWi, wo, wm, a2);

            // -- reflectance from the importance sampled GGX is interpolated based on metalness
            reflectance = surface.metalness * F * (G2 / G1);

            // -- reflectance from diffuse is blended based on (1 - metalness) and uses the Disney diffuse model.
            // -- http://blog.selfshadow.com/publications/s2015-shading-course/burley/s2015_pbs_disney_bsdf_notes.pdf
            float dotHL = BsdfNDot(localWi);
            float dotHV = BsdfNDot(wo);
            float rr = 0.5f + 2.0f * dotHL * dotHL * a;
            float fl = SchlickFresnel(dotHL);
            float fv = SchlickFresnel(dotHV);
            
            float3 fLambert = Math::OOPi_ * surface.albedo;
            float3 fRetro = fLambert * rr * (fl + fv + fl * fv * (rr - 1.0f));
            float3 diffuse = fLambert * (1.0f - 0.5f * fl)*(1.0f - 0.5f * fv) + fRetro;

            reflectance = reflectance + (1.0f - surface.metalness) * diffuse;

            wi = Math::Quaternion::Rotate(toWorld, localWi);
        }
    }
}