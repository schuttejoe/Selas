
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

namespace Selas
{
    //==============================================================================
    static float BsdfNDot(float3 x)
    {
        return x.y;
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
    bool SampleIblWithDisneyBrdf(GIIntegrationContext* __restrict context, const SurfaceParameters& surface, float3 v, BsdfSample& sample)
    {
        float r0 = Random::MersenneTwisterFloat(context->twister);
        float r1 = Random::MersenneTwisterFloat(context->twister);

        float theta;
        float phi;
        uint x;
        uint y;
        float iblPdf;
        Ibl(&context->sceneData->ibl->densityfunctions, r0, r1, theta, phi, x, y, iblPdf);
        float3 worldWi = Math::SphericalToCartesian(theta, phi);

        if(Dot(worldWi, surface.geometricNormal) < 0.0f) {
            return false;
        }

        float bsdfForwardPdf;
        float bsdfReversePdf;
        sample.reflectance = EvaluateDisneyBrdf(surface, v, worldWi, bsdfForwardPdf, bsdfReversePdf) * (1.0f / iblPdf);
        sample.forwardPdfW = iblPdf;
        sample.reversePdfW = iblPdf;
        sample.wi = worldWi;

        return true;
    }

    //==============================================================================
    bool SampleDisneyBrdf(GIIntegrationContext* __restrict context, const SurfaceParameters& surface, float3 v, BsdfSample& sample)
    {
        float3 wo = Normalize(MatrixMultiply(v, surface.worldToTangent));

        float a = surface.roughness;
        float a2 = a * a;

        float r0 = Random::MersenneTwisterFloat(context->twister);
        float r1 = Random::MersenneTwisterFloat(context->twister);
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