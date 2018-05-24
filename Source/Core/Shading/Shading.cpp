
//==============================================================================
// Joe Schutte
//==============================================================================

#include <Shading/Shading.h>
#include <Shading/SurfaceParameters.h>
#include <Shading/IntegratorContexts.h>

#include <Shading/Disney.h>
#include <Shading/TransparentGGX.h>

#include <Bsdf/Fresnel.h>
#include <Bsdf/Ggx.h>
#include <SceneLib/SceneResource.h>
#include <SceneLib/ImageBasedLightResource.h>
#include <GeometryLib/RectangulerLightSampler.h>
#include <GeometryLib/CoordinateSystem.h>
#include <UtilityLib/Color.h>
#include <MathLib/FloatFuncs.h>
#include <MathLib/FloatStructs.h>
#include <MathLib/Trigonometric.h>
#include <MathLib/ImportanceSampling.h>
#include <MathLib/Random.h>
#include <MathLib/Projection.h>
#include <MathLib/Quaternion.h>
#include <MathLib/GeometryIntersection.h>
#include <ContainersLib/Rect.h>
#include <SystemLib/MemoryAllocation.h>
#include <SystemLib/BasicTypes.h>
#include <SystemLib/MinMax.h>

#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>

namespace Shooty
{
    //==============================================================================
    bool SampleBsdfFunction(KernelContext* context, const SurfaceParameters& surface, float3 v, BsdfSample& sample)
    {
        // JSTODO - Rename these functions to be *Sample
        if(surface.shader == eDisney) {
            return DisneyBrdfShader(context, surface, v, sample);
        }
        else if(surface.shader == eTransparentGgx) {
            return TransparentGgxShader(context, surface, v, sample);
        }
        else {
            Assert_(false);
        }

        return false;
    }

    //==============================================================================
    float3 EvaluateBsdf(const SurfaceParameters& surface, float3 wo, float3 wi, float& forwardPdf, float& reversePdf)
    {
        // JSTODO - Rename these functions to be *Evaluate
        if(surface.shader == eDisney) {
            return CalculateDisneyBsdf(surface, wo, wi, forwardPdf, reversePdf);
        }
        else if(surface.shader == eTransparentGgx) {
            return CalculateTransparentGGXBsdf(surface, wo, wi, forwardPdf, reversePdf);
        }
        else {
            Assert_(false);
        }

        return float3::Zero_;
    }
}