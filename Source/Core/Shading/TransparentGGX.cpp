
//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "Shading/Disney.h"
#include "Shading/SurfaceScattering.h"
#include "Shading/SurfaceParameters.h"
#include "Shading/IntegratorContexts.h"

#include "Shading/Fresnel.h"
#include "Shading/Ggx.h"
#include "MathLib/FloatFuncs.h"
#include "MathLib/Trigonometric.h"

namespace Selas
{
    //=============================================================================================================================
    float3 EvaluateTransparentGGXBsdf(const SurfaceParameters& surface, const float3& v, const float3& l,
                                      float& forwardPdf, float& reversePdf)
    {
        // JSTODO - This was wrong!!! See disney.cpp for how to handle Transmission.
        return float3::Zero_;
    }

    //=============================================================================================================================
    bool SampleTransparentGgx(CSampler* sampler, const SurfaceParameters& surface, const float3& v, BsdfSample& sample)
    {
        // JSTODO - This was wrong!!! See disney.cpp for how to handle Transmission.
        return false;
    }
}