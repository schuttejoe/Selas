
//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "Shading/DiracTransparent.h"
#include "Shading/SurfaceScattering.h"
#include "Shading/SurfaceParameters.h"
#include "Shading/IntegratorContexts.h"
#include "Shading/Ggx.h"
#include "Shading/Fresnel.h"
#include "MathLib/FloatFuncs.h"
#include "MathLib/Trigonometric.h"

namespace Selas
{
    using namespace Math;

    //=============================================================================================================================
    bool SampleDiracTransparent(CSampler* sampler, const SurfaceParameters& surface, float3 v, BsdfSample& sample)
    {
        float3 normal = GeometricNormal(surface);

        float dotVH = Dot(v, normal);
        float ni = dotVH > 0.0f ? 1.0f : surface.ior;
        float nt = dotVH > 0.0f ? surface.ior : 1.0f;

        float n = ni / nt;

        float F = Fresnel::Dielectric(dotVH, ni, nt);
        float pdf;

        float3 wi;
        if(sampler->UniformFloat() < F) {
            wi = Normalize(Reflect(normal, v));
            sample.flags = eScatterEvent;

            float dotNL = AbsCosTheta(wi);
            float jacobian = (4 * dotNL);

            pdf = F;
        }
        else {
            if(Transmit(normal, v, n, wi)) {
                sample.flags = eTransmissionEvent;
            }
            else {
                Assert_(nt < ni);
                wi = Reflect(normal, v);
                sample.flags = eScatterEvent;
            }

            wi = Normalize(wi);

            float dotLH = Absf(Dot(wi, float3::YAxis_));
            float jacobian = dotLH * 1.0f / (Square(dotLH + surface.ior * dotVH));

            pdf = (1.0f - F);
        }

        sample.flags |= eDeltaEvent;
        sample.reflectance = surface.baseColor / pdf;
        sample.wi = wi;
        sample.forwardPdfW = pdf;
        sample.reversePdfW = pdf;        

        return true;
    }
}
