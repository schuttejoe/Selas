
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
            if(surface.shader == eDisneyThin) {
                return SampleDisney(sampler, surface, v, true, sample);
            }
            if(surface.shader == eDisneySolid) {
                return SampleDisney(sampler, surface, v, false, sample);
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
            if(surface.shader == eDisneyThin) {
                return EvaluateDisney(surface, v, l, true, forwardPdfW, reversePdfW);
            }
            if(surface.shader == eDisneySolid) {
                return EvaluateDisney(surface, v, l, false, forwardPdfW, reversePdfW);
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