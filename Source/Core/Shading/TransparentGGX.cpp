
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

namespace Selas
{
    //==============================================================================
    static float BsdfNDot(float3 x)
    {
        return x.y;
    }

    //==============================================================================
    float3 EvaluateTransparentGGXBsdf(const SurfaceParameters& surface, float3 wo, float3 wi, float& forwardPdf, float& reversePdf)
    {
        // JSTODO - validate me
        float3 wm = Normalize(wo + wi);

        float a = surface.roughness;
        float a2 = a * a;
        float absDotNH = Math::Absf(Dot(surface.perturbedNormal, wm));
        float absDotNL = Math::Absf(Dot(surface.perturbedNormal, wi));
        float absDotNV = Math::Absf(Dot(surface.perturbedNormal, wo));
        float absDotHV = Math::Absf(Dot(wm, wo));
        float absDotHL = Math::Absf(Dot(wm, wi));

        float F = (1.0f - Fresnel::SchlickDialectic(absDotHV, surface.currentIor, surface.exitIor));
        float G2 = Bsdf::SmithGGXMaskingShading(absDotNL, absDotNV, a2);

        float denomPart = (absDotNH * absDotNL) * (a2 - 1) + 1;
        float D = a2 / (Math::Pi_ * denomPart * denomPart);

        forwardPdf = Bsdf::GgxVndfPdf(absDotHV, absDotNL, absDotNV, absDotNH, a2);
        reversePdf = Bsdf::GgxVndfPdf(absDotHL, absDotNV, absDotNL, absDotNH, a2);

        return F * G2 * D;
    }

    //==============================================================================
    bool SampleTransparentGgx(KernelContext* __restrict context, const SurfaceParameters& surface, float3 v, BsdfSample& sample)
    {
        float3 wo = Normalize(MatrixMultiply(v,  surface.worldToTangent));
        float3x3 tangentToWorld = MatrixTranspose(surface.worldToTangent);

        float a = surface.roughness;
        float a2 = a * a;

        float r0 = Random::MersenneTwisterFloat(context->twister);
        float r1 = Random::MersenneTwisterFloat(context->twister);
        float3 wm = Bsdf::GgxVndf(wo, surface.roughness, r0, r1);

        bool usedReflection = false;

        float3 wi;
        float t = Random::MersenneTwisterFloat(context->twister);
        float F = Fresnel::SchlickDialectic(Math::Absf(Dot(wm, wo)), surface.currentIor, surface.exitIor);
        float scatterPdf = 1.0f - F;
        if(t < F || !Transmit(wm, wo, surface.currentIor, surface.exitIor, wi)) {
            wi = Reflect(wm, wo);
            usedReflection = true;
            scatterPdf = F;
        }
        wi = Normalize(wi);

        float absDotNL = Math::Absf(BsdfNDot(wi));
        float absDotNV = Math::Absf(BsdfNDot(wo));
        float absDotNH = Math::Absf(BsdfNDot(wm));
        float absDotHV = Math::Absf(Dot(wm, wo));
        float absDotHL = Math::Absf(Dot(wm, wi));

        float G1 = Bsdf::SmithGGXMasking(absDotNV, a2);
        float G2 = Bsdf::SmithGGXMaskingShading(absDotNL, absDotNV, a2);

        sample.reflectance = surface.albedo * (G2 / G1);
        sample.reflection = usedReflection;
        sample.wi = Normalize(MatrixMultiply(wi, tangentToWorld));
        sample.forwardPdfW = scatterPdf * Bsdf::GgxVndfPdf(absDotHV, absDotNL, absDotNV, absDotNH, a2);
        sample.reversePdfW = scatterPdf * Bsdf::GgxVndfPdf(absDotHL, absDotNV, absDotNL, absDotNH, a2);

        return true;
    }
}