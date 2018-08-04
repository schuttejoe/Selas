
//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "Shading/Lambert.h"
#include "Shading/Lighting.h"
#include "Shading/SurfaceParameters.h"
#include "Shading/IntegratorContexts.h"

#include "MathLib/FloatFuncs.h"
#include "MathLib/Trigonometric.h"
#include "SystemLib/MinMax.h"

namespace Selas
{
    //=============================================================================================================================
    static float3 SampleCosineWeightedHemisphere(float r0, float r1)
    {
        float r = Math::Sqrtf(r0);
        float theta = Math::TwoPi_ * r1;

        return float3(r * Math::Cosf(theta), Math::Sqrtf(Max(0.0f, 1 - r0)), r * Math::Sinf(theta));
    }

    //=============================================================================================================================
    float3 EvaluateLambert(const SurfaceParameters& surface, const float3& v, const float3& l,
                           float& forwardPdf, float& reversePdf)
    {
        forwardPdf = 0.0f;
        reversePdf = 0.0f;

        float3 N = GeometricNormal(surface);

        float dotNL = Dot(N, l);
        float dotNV = Dot(N, v);
        if(dotNL <= 0.0f || dotNV <= 0.0f) {
            return float3::Zero_;
        }

        forwardPdf = dotNL;
        reversePdf = dotNV;
        return surface.baseColor * Math::InvPi_ * dotNL;
    }

    //=============================================================================================================================
    bool SampleLambert(CSampler* sampler, const SurfaceParameters& surface, const float3& v, BsdfSample& sample)
    {
        float3 wo = MatrixMultiply(v, surface.worldToTangent);

        // -- Sample cosine lobe
        float r0 = sampler->UniformFloat();
        float r1 = sampler->UniformFloat();
        float3 wi = SampleCosineWeightedHemisphere(r0, r1);

        float dotNL = Math::CosTheta(wi);
        float dotNV = Math::CosTheta(wo);

        sample.reflectance = surface.baseColor * Math::InvPi_;
        sample.wi = Normalize(MatrixMultiply(wi, MatrixTranspose(surface.worldToTangent)));
        sample.forwardPdfW = dotNL;
        sample.reversePdfW = dotNV;
        sample.type = SurfaceEventTypes::eScatterEvent;

        return true;
    }
}