
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

namespace Shooty
{
    //==============================================================================
    float3 CalculateTransparentGGXBsdf(const SurfaceParameters& surface, float3 wo, float3 wi, float pdf)
    {
        // JSTODO - validate me
        float3 wm = Normalize(wo + wi);

        float a = surface.roughness;
        float a2 = a * a;
        float dotNH = Dot(surface.perturbedNormal, wm);
        float dotNL = Dot(surface.perturbedNormal, wi);
        if(dotNL <= 0.0f || dotNH <= 0.0f) {
            return float3::Zero_;
        }

        float F = (1.0f - Fresnel::SchlickDialectic(Math::Absf(Dot(wm, wo)), surface.currentIor, surface.exitIor));
        float G2 = Bsdf::SmithGGXMaskingShading(wi, wo, wm, a2);

        float denomPart = (dotNH * dotNH) * (a2 - 1) + 1;
        float D = a2 / (Math::Pi_ * denomPart * denomPart);

        pdf = Bsdf::GgxVndfPdf(a, wo, wm, wi);

        return F * G2 * D ;
    }

    //==============================================================================
    void TransparentGgxShader(KernelContext* __restrict context, const HitParameters& hit, const SurfaceParameters& surface)
    {
        float3 view = hit.viewDirection;
        float3 wo = Normalize(MatrixMultiply(view, surface.worldToTangent));

        float a = surface.roughness;
        float a2 = a * a;

        float r0 = Random::MersenneTwisterFloat(context->twister);
        float r1 = Random::MersenneTwisterFloat(context->twister);
        float3 wm = Bsdf::GgxVndf(wo, surface.roughness, r0, r1);

        bool usedRefraction = true;

        float3 wi;
        float t = Random::MersenneTwisterFloat(context->twister);
        float F = Fresnel::SchlickDialectic(Math::Absf(Dot(wm, wo)), surface.currentIor, surface.exitIor);
        if(t < F || !Transmit(wm, wo, surface.currentIor, surface.exitIor, wi)) {
            wi = Reflect(wm, wo);
            usedRefraction = false;
        }
        wi = Normalize(wi);

        float G1 = Bsdf::SmithGGXMasking(wi, wo, wm, a2);
        float G2 = Bsdf::SmithGGXMaskingShading(wi, wo, wm, a2);

        float3 reflectance = surface.albedo * (G2 / G1);

        float3 worldWi = Normalize(MatrixMultiply(wi, surface.tangentToWorld));
        Ray bounceRay;
        if(usedRefraction)
            bounceRay = CreateRefractionBounceRay(surface, hit, worldWi, reflectance, surface.currentIor / surface.exitIor);
        else
            bounceRay = CreateReflectionBounceRay(surface, hit, worldWi, reflectance);

        InsertRay(context, bounceRay);
    }
}