
//==============================================================================
// Joe Schutte
//==============================================================================

#include "PathTracerShading.h"
#include "SurfaceParameters.h"

#include <SceneLib/SceneResource.h>
#include <SceneLib/ImageBasedLightResource.h>
#include <GeometryLib/RectangulerLight.h>
#include <GeometryLib/CoordinateSystem.h>
#include <UtilityLib/Color.h>
#include <MathLib/FloatFuncs.h>
#include <MathLib/FloatStructs.h>
#include <MathLib/Trigonometric.h>
#include <MathLib/ImportanceSampling.h>
#include <MathLib/Random.h>
#include <MathLib/Projection.h>
#include <MathLib/Quaternion.h>
#include <MathLib/GeometryIntersection.h>
#include <ContainersLib/Rect.h>
#include <SystemLib/MemoryAllocation.h>
#include <SystemLib/BasicTypes.h>
#include <SystemLib/MinMax.h>

#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>

namespace Shooty
{
    //==============================================================================
    static bool OcclusionRay(RTCScene& rtcScene, const SurfaceParameters& surface, float3 direction, float distance)
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
        
        // -- remap from [-pi,pi] to [0, 2pi]
        phi += Math::Pi_;

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

    ////==============================================================================
    //void ImportanceSampleGgxD(Random::MersenneTwister* twister, float3 wg, float3 v, Material* material, float3& wi, float3& reflectance)
    //{
    //    // -- build tangent space transforms // -- JSTODO - get tangent space from mesh
    //    float4 toLocal = ToTangentSpaceQuaternion(wg);
    //    float4 toWorld = Math::Quaternion::Invert(toLocal);

    //    float3 wo = Math::Quaternion::Rotate(toLocal, v);

    //    float theta;
    //    float phi;
    //    float r0 = Random::MersenneTwisterFloat(twister);
    //    float r1 = Random::MersenneTwisterFloat(twister);
    //    ImportanceSampling::GgxD(material->roughness, r0, r1, theta, phi);

    //    float3 wm = Math::SphericalToCartesian(theta, phi);
    //    wi = Reflect(wm, wo);

    //    if(BsdfNDot(wi) > 0.0f && Dot(wi, wm) > 0.0f) {
    //        float a = material->roughness;
    //        float a2 = a * a;

    //        float3 F = SchlickFresnel(material->specularColor, Dot(wi, wm));
    //        float G = SmithGGXMaskingShading(wi, wo, wm, a2);
    //        float weight = Math::Absf(Dot(wo, wm)) / (BsdfNDot(wo) * BsdfNDot(wm));

    //        reflectance = F * G * weight;
    //        wi = Math::Quaternion::Rotate(toWorld, wi);    
    //    }
    //    else {
    //        reflectance = float3::Zero_;
    //    }
    //}

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
    static float3 CalculateDisneyBsdf(const SurfaceParameters& surface, float3 wo, float3 wi)
    {
        float3 wm = Normalize(wo + wi);

        float a = surface.roughness;
        float a2 = a * a;
        float dotLH = Dot(wi, wm);
        float dotNH = Dot(surface.normal, wm);
        float dotNL = Dot(surface.normal, wi);
        float dotNV = Dot(surface.normal, wo);
        if(dotNL <= 0.0f || dotNV <= 0.0f || dotNH <= 0.0f) {
            return float3::Zero_;
        }

        float3 F = SchlickFresnel(surface.specularColor, dotLH);

        float G1 = SmithGGXMasking(wi, wo, wm, a2);
        float G2 = SmithGGXMaskingShading(wi, wo, wm, a2);

        float denomPart = (dotNH * dotNH) * (a2 - 1) + 1;
        float D = a2 / (Math::Pi_ * denomPart * denomPart);

        // -- reflectance from diffuse is blended based on (1 - metalness) and uses the Disney diffuse model.
        // -- http://blog.selfshadow.com/publications/s2015-shading-course/burley/s2015_pbs_disney_bsdf_notes.pdf
        float rr = 0.5f + 2.0f * dotNL * dotNL * a;
        float fl = SchlickFresnel(dotNL);
        float fv = SchlickFresnel(dotNV);

        float3 fLambert = Math::OOPi_ * surface.albedo;
        float3 fRetro = fLambert * rr * (fl + fv + fl * fv * (rr - 1.0f));
        float3 diffuse = fLambert * (1.0f - 0.5f * fl)*(1.0f - 0.5f * fv) + fRetro;

        return surface.metalness * F * (G2 / G1) * D + (1.0f - surface.metalness) * diffuse;
    }

    //==============================================================================
    static float3 IntegrateRectangleLightWithArea(RTCScene& rtcScene, Random::MersenneTwister* twister, const SurfaceParameters& surface, RectangularAreaLight light, uint sampleCount)
    {
        float3 eX = light.eX;
        float3 eZ = light.eZ;
        float3 s = light.corner;

        float eXLength = Length(eX);
        float eZLength = Length(eZ);

        float3 lightFacing = Normalize(Cross(eX, eZ));

        if(Dot(-lightFacing, surface.normal) <= 0.0f) {
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

            float dotNL = Saturate(Dot(surface.normal, l));
            float dotSL = Saturate(Dot(lightFacing, -l));

            if(dotNL > 0.0f && dotSL > 0.0f && OcclusionRay(rtcScene, surface, l, dist)) {
                Lo += (dotNL * dotSL / distSquared) * light.intensity;
            }
        }

        return Lo * (1.0f / (pdf * sampleCount));
    }

    //==============================================================================
    static float3 IntegrateRectangleLightWithSolidAngle(RTCScene& rtcScene, Random::MersenneTwister* twister, const SurfaceParameters& surface, RectangularAreaLight light, uint sampleCount)
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

            float dotNL = Saturate(Dot(surface.normal, l));
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
    static float3 IntegrateSphereLightWithAreaSampling(RTCScene& rtcScene, Random::MersenneTwister* twister, const SurfaceParameters& surface, SphericalAreaLight light, uint lightSampleCount)
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
            float dotNL = Dot(surface.normal, w);

            if(dotSL > 0.0f && dotNL > 0.0f && OcclusionRay(rtcScene, surface, w, dist)) {
                Lo += L * dotNL * dotSL * (1.0f / distSquared);
            }
        }

        return Lo * (1.0f / (lightSampleCount * pdf));
    }

    //==============================================================================
    static float3 IntegrateSphereLightWithSolidAngleSampling(RTCScene& rtcScene, Random::MersenneTwister* twister, const SurfaceParameters& surface, float3 v, SphericalAreaLight light, uint lightSampleCount)
    {
        float3 L = light.intensity;
        float3 c = light.center;
        float  r = light.radius;

        float3 w = c - surface.position;
        float distanceToCenter = Length(w);
        w = w * (1.0f / distanceToCenter);

        float q = Math::Sqrtf(1.0f - (r / distanceToCenter) * (r / distanceToCenter));

        float3 n = surface.normal;
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

            float dotNL = Saturate(Dot(nwp, surface.normal));
            if(dotNL > 0.0f && OcclusionRay(rtcScene, surface, nwp, dist)) {
                // -- the dist^2 and Dot(w', n') terms from the pdf and the area form of the rendering equation cancel out
                float pdf_xp = 1.0f / (Math::TwoPi_ * (1.0f - q));
                float3 bsdf = CalculateDisneyBsdf(surface, v, nwp);
                Lo += bsdf * (1.0f / pdf_xp) * L;
            }
        }

        return Lo * (1.0f / lightSampleCount);
    }

    //==============================================================================
    float3 CalculateDirectLighting(RTCScene& rtcScene, SceneResource* scene, Random::MersenneTwister* twister, const SurfaceParameters& surface, float3 v)
    {
        // -- JSTODO - find light sources that can effect this point

        uint lightSampleCount = 1;

        float3 lightCenter = float3(0.0f, 10.0f, 0.0f);
        float3 lightIntensity = float3(10.0f, 10.0f, 10.0f);

        RectangularAreaLight rectangleLight;
        rectangleLight.intensity = lightIntensity;
        rectangleLight.eX = float3(5.0f, 0.0f, 0.0f);
        rectangleLight.eZ = float3(0.0f, 0.0f, 5.0f);
        rectangleLight.corner = lightCenter - 0.5f * rectangleLight.eX - 0.5f * rectangleLight.eZ;

        SphericalAreaLight sphereLight;
        sphereLight.intensity = lightIntensity;
        sphereLight.center = lightCenter;
        sphereLight.radius = 1.9947114f; // chosen to have a surface area of 50

        float3 sa = IntegrateSphereLightWithSolidAngleSampling(rtcScene, twister, surface, v, sphereLight, lightSampleCount);
        return sa;
    }

    //==============================================================================
    void ImportanceSampleGgxVdn(Random::MersenneTwister* twister, const SurfaceParameters& surface, float3 wo, float3& wi, float3& reflectance)
    {
        float a = surface.roughness;
        float a2 = a * a;

        float r0 = Random::MersenneTwisterFloat(twister);
        float r1 = Random::MersenneTwisterFloat(twister);
        float3 wm = ImportanceSampling::GgxVndf(wo, surface.roughness, r0, r1);

        wi = Normalize(Reflect(wm, wo));

        if(BsdfNDot(wi) > 0.0f) {
            float3 F = SchlickFresnel(surface.specularColor, Dot(wi, wm));
            float G1 = SmithGGXMasking(wi, wo, wm, a2);
            float G2 = SmithGGXMaskingShading(wi, wo, wm, a2);

            reflectance = F * (G2 / G1);
            
        }
        else {
            reflectance = float3::Zero_;
        }
    }

    //==============================================================================
    void ImportanceSampleDisneyBrdf(Random::MersenneTwister* twister, const SurfaceParameters& surface, float3 wo, float3& wi, float3& reflectance)
    {
        float a = surface.roughness;
        float a2 = a * a;

        float r0 = Random::MersenneTwisterFloat(twister);
        float r1 = Random::MersenneTwisterFloat(twister);
        float3 wm = ImportanceSampling::GgxVndf(wo, surface.roughness, r0, r1);

        wi = Reflect(wm, wo);
        wi = Normalize(wi);

        reflectance = float3::Zero_;

        if(BsdfNDot(wi) > 0.0f) {
            float3 F = SchlickFresnel(surface.specularColor, Dot(wi, wm));
            float G1 = SmithGGXMasking(wi, wo, wm, a2);
            float G2 = SmithGGXMaskingShading(wi, wo, wm, a2);

            // -- reflectance from the importance sampled GGX is interpolated based on metalness
            reflectance = surface.metalness * F * (G2 / G1);

            // -- reflectance from diffuse is blended based on (1 - metalness) and uses the Disney diffuse model.
            // -- http://blog.selfshadow.com/publications/s2015-shading-course/burley/s2015_pbs_disney_bsdf_notes.pdf
            float dotNL = BsdfNDot(wi);
            float dotNV = BsdfNDot(wo);
            float rr = 0.5f + 2.0f * dotNL * dotNL * a;
            float fl = SchlickFresnel(dotNL);
            float fv = SchlickFresnel(dotNV);
            
            float3 fLambert = Math::OOPi_ * surface.albedo;
            float3 fRetro = fLambert * rr * (fl + fv + fl * fv * (rr - 1.0f));
            float3 diffuse = fLambert * (1.0f - 0.5f * fl)*(1.0f - 0.5f * fv) + fRetro;

            reflectance = reflectance + (1.0f - surface.metalness) * diffuse;
        }
    }
}