
//==============================================================================
// Joe Schutte
//==============================================================================

#include "Shading/Disney.h"
#include "Shading/Lighting.h"
#include "Shading/SurfaceParameters.h"
#include "Shading/IntegratorContexts.h"

#include "Bsdf/Fresnel.h"
#include "Bsdf/Ggx.h"
#include "MathLib/FloatFuncs.h"
#include "MathLib/Trigonometric.h"
#include "MathLib/Projection.h"
#include "SystemLib/MinMax.h"

namespace Selas
{
    // Burley 2012
    // http://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf
    // Burley 2015
    // http://blog.selfshadow.com/publications/s2015-shading-course/burley/s2015_pbs_disney_bsdf_notes.pdf
    // Hanrahan-Krueger 1993
    // https://cseweb.ucsd.edu/~ravir/6998/papers/p165-hanrahan.pdf
    // Disney BRDF explorer
    // https://github.com/wdas/brdf

    //==============================================================================
    static float BsdfNDot(float3 x)
    {
        return x.y;
    }

    //==============================================================================
    static float3 DisneyDiffuse(const float3& baseColor, float absDotNL, float absDotNV, float dotIH)
    {
        float fl = Fresnel::Schlick(absDotNL);
        float fv = Fresnel::Schlick(absDotNV);

        return baseColor * Math::OOPi_ * (1.0f - 0.5f * fl) * (1.0f - 0.5f * fv);
    }

    //==============================================================================
    static float3 HanrahanKrueger(const float3& baseColor, float roughness, float absDotNL, float absDotNV, float dotHL)
    {
        float fss90 = dotHL * dotHL * roughness;
        float fl = Fresnel::Schlick(absDotNL);
        float fv = Fresnel::Schlick(absDotNV);

        float fss = Lerp(1.0f, fss90, fl) * Lerp(1.0f, fss90, fv);

        float ss = 1.25f * (fss * (1.0f / (absDotNL + absDotNV) - 0.5f) + 0.5f);
        return baseColor * Math::OOPi_ * ss;
    }

    //==============================================================================
    static float3 DisneyRetro(const float3& baseColor, float roughness, float absDotNL, float absDotNV, float dotHL)
    {
        float fl = Fresnel::Schlick(absDotNL);
        float fv = Fresnel::Schlick(absDotNV);

        float retro = 2.0f * roughness * dotHL * dotHL;
        return baseColor * Math::OOPi_ * retro * (fl + fv + fl * fv * (retro - 1.0f));
    }

    //==============================================================================
    static float3 DisneySheen(const float3& baseColor, float dotHL)
    {
        return baseColor * Fresnel::Schlick(dotHL);
    }

    //==============================================================================
    // -- "generalized" Trowbridge-Reitz curve ungeneralized with a hard-coded exponent of 1
    static float GTR1(float absDotHL, float a2)
    {
        return (a2 - 1.0f) / (Math::Pi_ * Math::Log2(a2) * (1.0f + (a2 - 1.0f) * absDotHL * absDotHL));
    }

    //==============================================================================
    static float CorrelatedSmithMasking(float cosTheta, float a2)
    {
        float cosTheta2 = cosTheta * cosTheta;
        return 1.0f / (cosTheta + Math::Sqrtf(a2 + cosTheta2 - a2 * cosTheta2));
    }

    //==============================================================================
    static float3 DisneyClearcoat(float weight, float gloss, float absDotNL, float absDotNV, float dotNH, float dotHL)
    {
        float d = GTR1(Math::Absf(dotNH), Lerp(0.1f, 0.001f, gloss));
        float f = Fresnel::Schlick(0.04f, dotHL);
        float g = CorrelatedSmithMasking(absDotNL, 0.25f) * CorrelatedSmithMasking(absDotNV, 0.25f);

        return 0.25f * weight * d * f * g;
    }

    //==============================================================================
    static bool SampleDisneyClearcoat(CSampler* sampler, const SurfaceParameters& surface, const float3& v, BsdfSample& sample)
    {
        float3 wo = Normalize(MatrixMultiply(v, surface.worldToTangent));

        float a = 0.25f;
        float a2 = a * a;

        float r0 = sampler->UniformFloat();
        float r1 = sampler->UniformFloat();
        float cosTheta = Math::Sqrtf(Max<float>(0, (1.0f - Math::Powf(a2, 1.0f - r0)) / (1.0f - a2)));
        float sinTheta = Math::Sqrtf(Max<float>(0, 1.0f - cosTheta * cosTheta));
        float phi = Math::TwoPi_ * r1;

        float3 wm = float3(sinTheta * Math::Cosf(phi), cosTheta, sinTheta * Math::Sinf(phi));
        if(Dot(wm, wo) < 0.0f) {
            wm = -wm;
        }

        float3 wi = Reflect(wm, wo);
        if(Dot(wi, wo) < 0.0f) {
            return false;
        }

        float clearcoatWeight = 1.0f;
        float clearcoatGloss = 0.1f;

        float dotNH = BsdfNDot(wm);
        float dotLH = Dot(wm, wi);
        float absDotNL = Math::Absf(BsdfNDot(wi));
        float absDotNV = Math::Absf(BsdfNDot(wo));

        float d = GTR1(Math::Absf(dotNH), Lerp(0.1f, 0.001f, clearcoatGloss));
        float f = Fresnel::Schlick(0.04f, dotLH);
        float g = CorrelatedSmithMasking(absDotNL, 0.25f) * CorrelatedSmithMasking(absDotNV, 0.25f);

        float3x3 tangentToWorld = MatrixTranspose(surface.worldToTangent);

        sample.reflectance = 0.25f * clearcoatWeight * g * f * d;
        sample.wi = Normalize(MatrixMultiply(wi, tangentToWorld));
        sample.forwardPdfW = d / (4.0f * Dot(wo, wm));
        sample.reversePdfW = d / (4.0f * Dot(wi, wm));

        return true;
    }

    //==============================================================================
    float3 EvaluateDisneyBrdf(const SurfaceParameters& surface, float3 wo, float3 wi, float& forwardPdf, float& reversePdf)
    {
        // JSTODO - validate me

        float a = surface.roughness;
        float a2 = a * a;

        float dotNV = Dot(surface.perturbedNormal, wo);
        float dotNL = Dot(surface.perturbedNormal, wi);
        if(dotNL <= 0.0f || dotNV <= 0.0f) {
            return float3::Zero_;
        }

        float3 wm = Normalize(wo + wi);
        float dotNH = Dot(surface.perturbedNormal, wm);
        float dotLH = Dot(wi, wm);

        float3 F = Fresnel::Schlick(surface.specularColor, dotLH);

        float G1 = Bsdf::SmithGGXMasking(dotNV, a2);
        float G2 = Bsdf::SmithGGXMaskingShading(dotNL, dotNV, a2);

        float denomPart = (dotNH * dotNH) * (a2 - 1) + 1;
        float D = a2 / (Math::Pi_ * denomPart * denomPart);

        // -- reflectance from diffuse is blended based on (1 - metalness) and uses the Disney diffuse model.
        // -- http://blog.selfshadow.com/publications/s2015-shading-course/burley/s2015_pbs_disney_bsdf_notes.pdf
        float rr = 0.5f + 2.0f * dotNL * dotNL * a;
        float fl = Fresnel::Schlick(dotNL);
        float fv = Fresnel::Schlick(dotNV);

        float3 fLambert = Math::OOPi_ * surface.albedo;
        float3 fRetro = fLambert * rr * (fl + fv + fl * fv * (rr - 1.0f));
        float3 diffuse = fLambert * (1.0f - 0.5f * fl)*(1.0f - 0.5f * fv) + fRetro;

        forwardPdf = Bsdf::GgxVndfPdf(dotLH, dotNL, dotNV, dotNH, a2);
        reversePdf = Bsdf::GgxVndfPdf(dotLH, dotNV, dotNL, dotNH, a2);

        return surface.metalness * F * (G2 / G1) * D + (1.0f - surface.metalness) * diffuse;
    }

    //==============================================================================
    bool SampleDisneyBrdf(CSampler* sampler, const SurfaceParameters& surface, float3 v, BsdfSample& sample)
    {
        float3 wo = Normalize(MatrixMultiply(v, surface.worldToTangent));

        float a = surface.roughness;
        float a2 = a * a;

        float r0 = sampler->UniformFloat();
        float r1 = sampler->UniformFloat();
        float3 wm = Bsdf::GgxVndf(wo, surface.roughness, r0, r1);

        float3 wi = Reflect(wm, wo);
        if(BsdfNDot(wi) > 0.0f) {
            wi = Normalize(wi);

            float dotNL = BsdfNDot(wi);
            float dotNV = BsdfNDot(wo);
            float dotNH = BsdfNDot(wm);
            
            float dotLH = Dot(wi, wm);

            float3 F = Fresnel::Schlick(surface.specularColor, Dot(wi, wm));
            float G1 = Bsdf::SmithGGXMasking(dotNV, a2);
            float G2 = Bsdf::SmithGGXMaskingShading(dotNL, dotNV, a2);

            // -- reflectance from diffuse is blended based on (1 - metalness) and uses the Disney diffuse model.
            // -- http://blog.selfshadow.com/publications/s2015-shading-course/burley/s2015_pbs_disney_bsdf_notes.pdf

            float rr = 0.5f + 2.0f * dotNL * dotNL * a;
            float fl = Fresnel::Schlick(dotNL);
            float fv = Fresnel::Schlick(dotNV);

            float3 fLambert = Math::OOPi_ * surface.albedo;
            float3 fRetro = fLambert * rr * (fl + fv + fl * fv * (rr - 1.0f));
            float3 diffuse = fLambert * (1.0f - 0.5f * fl)*(1.0f - 0.5f * fv) + fRetro;

            float3x3 tangentToWorld = MatrixTranspose(surface.worldToTangent);

            sample.reflectance = surface.metalness * F * (G2 / G1) + (1.0f - surface.metalness) * diffuse;
            sample.wi = Normalize(MatrixMultiply(wi, tangentToWorld));
            sample.forwardPdfW = Bsdf::GgxVndfPdf(dotLH, dotNL, dotNV, dotNH, a2);
            sample.reversePdfW = Bsdf::GgxVndfPdf(dotLH, dotNV, dotNL, dotNH, a2);
            return true;
        }

        return false;
    }
}