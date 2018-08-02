
//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "Shading/Lighting.h"
#include "Shading/SurfaceParameters.h"
#include "Shading/Lambert.h"
#include "Shading/Disney.h"
#include "Shading/TransparentGGX.h"
#include "SystemLib/JsAssert.h"

// JSTODO - Rename this to shading

namespace Selas
{
    #define LambertAllTheThings_ 0

    //=============================================================================================================================
    bool SampleBsdfFunction(CSampler* sampler, const SurfaceParameters& surface, float3 v, BsdfSample& sample)
    {
        #if LambertAllTheThings_
            return SampleLambert(sampler, surface, v, sample);
        #else
            if(surface.shader == eDisney) {
                return SampleDisneyThin(sampler, surface, v, sample);
            }
            else if(surface.shader == eTransparentGgx) {
                return SampleTransparentGgx(sampler, surface, v, sample);
            }
            else {
                Assert_(false);
            }
        #endif

        return false;
    }

    //=============================================================================================================================
    float3 EvaluateBsdf(const SurfaceParameters& surface, float3 v, float3 l, float& forwardPdfW, float& reversePdfW)
    {
        #if LambertAllTheThings_
            return EvaluateLambert(surface, v, l, forwardPdfW, reversePdfW);
        #else
            if(surface.shader == eDisney) {
                return EvaluateDisneyThin(surface, v, l, forwardPdfW, reversePdfW);
            }
            else if(surface.shader == eTransparentGgx) {
                return EvaluateTransparentGGXBsdf(surface, v, l, forwardPdfW, reversePdfW);
            }
            else {
                Assert_(false);
            }
        #endif

        return float3::Zero_;
    }
}