
//==============================================================================
// Joe Schutte
//==============================================================================

#include <Shading/Disney.h>
#include <Shading/Shading.h>
#include <Shading/SurfaceParameters.h>
#include <Shading/IntegratorContexts.h>

#include <Bsdf/Fresnel.h>
#include <Bsdf/Ggx.h>
#include <MathLib/FloatFuncs.h>
#include <MathLib/Trigonometric.h>
#include <MathLib/Projection.h>

namespace Shooty
{
    //==============================================================================
    static float BsdfNDot(float3 x)
    {
        return x.y;
    }

    //==============================================================================
    float3 CalculateDisneyBsdf(const SurfaceParameters& surface, float3 wo, float3 wi, float& pdf)
    {
        float3 wm = Normalize(wo + wi);

        float a = surface.roughness;
        float a2 = a * a;
        float dotLH = Dot(wi, wm);
        float dotNH = Dot(surface.perturbedNormal, wm);
        float dotNL = Dot(surface.perturbedNormal, wi);
        float dotNV = Dot(surface.perturbedNormal, wo);
        if(dotNL <= 0.0f || dotNH <= 0.0f) {
            return float3::Zero_;
        }

        float3 F = Fresnel::Schlick(surface.specularColor, dotLH);

        float G1 = Bsdf::SmithGGXMasking(wi, wo, wm, a2);
        float G2 = Bsdf::SmithGGXMaskingShading(wi, wo, wm, a2);

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

        pdf = Bsdf::GgxVndfPdf(a, wo, wm, wi);

        return surface.metalness * F * (G2 / G1) * D + (1.0f - surface.metalness) * diffuse;
    }

    //==============================================================================
    void DisneyWithIblSamplingShader(KernelContext* __restrict context, const HitParameters& hit, const SurfaceParameters& surface)
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
            return;
        }

        float bsdfPdf;
        float3 reflectance = CalculateDisneyBsdf(surface, hit.viewDirection, worldWi, bsdfPdf) * (1.0f / iblPdf);
        Ray bounceRay = CreateReflectionBounceRay(surface, hit, worldWi, reflectance);
        InsertRay(context, bounceRay);
    }

    //==============================================================================
    void DisneyBrdfShader(KernelContext* __restrict context, const HitParameters& hit, const SurfaceParameters& surface)
    {
        float3 wo = Normalize(MatrixMultiply(hit.viewDirection, surface.worldToTangent));

        float a = surface.roughness;
        float a2 = a * a;

        float r0 = Random::MersenneTwisterFloat(context->twister);
        float r1 = Random::MersenneTwisterFloat(context->twister);
        float3 wm = Bsdf::GgxVndf(wo, surface.roughness, r0, r1);

        float3 wi = Reflect(wm, wo);
        if(BsdfNDot(wi) > 0.0f) {
            wi = Normalize(wi);

            float3 F = Fresnel::Schlick(surface.specularColor, Dot(wi, wm));
            float G1 = Bsdf::SmithGGXMasking(wi, wo, wm, a2);
            float G2 = Bsdf::SmithGGXMaskingShading(wi, wo, wm, a2);

            // -- reflectance from diffuse is blended based on (1 - metalness) and uses the Disney diffuse model.
            // -- http://blog.selfshadow.com/publications/s2015-shading-course/burley/s2015_pbs_disney_bsdf_notes.pdf
            float dotNL = BsdfNDot(wi);
            float dotNV = BsdfNDot(wo);
            float rr = 0.5f + 2.0f * dotNL * dotNL * a;
            float fl = Fresnel::Schlick(dotNL);
            float fv = Fresnel::Schlick(dotNV);

            float3 fLambert = Math::OOPi_ * surface.albedo;
            float3 fRetro = fLambert * rr * (fl + fv + fl * fv * (rr - 1.0f));
            float3 diffuse = fLambert * (1.0f - 0.5f * fl)*(1.0f - 0.5f * fv) + fRetro;

            float3 reflectance = surface.metalness * F * (G2 / G1) + (1.0f - surface.metalness) * diffuse;

            float3 worldWi = Normalize(MatrixMultiply(wi, surface.tangentToWorld));
            Ray bounceRay = CreateReflectionBounceRay(surface, hit, worldWi, reflectance);
            InsertRay(context, bounceRay);
        }
    }
}