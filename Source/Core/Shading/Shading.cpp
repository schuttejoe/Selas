
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
#include <GeometryLib/RectangulerLight.h>
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
    void SampleBsdfFunction(KernelContext* context, const SurfaceParameters& surface, BsdfSample& sample)
    {
        if(surface.shader == eDisney) {
            //DisneyBrdfShader(context, surface, sample);
            DisneyWithIblSamplingShader(context, surface, sample);
        }
        else if(surface.shader == eTransparentGgx) {
            TransparentGgxShader(context, surface, sample);
        }
        else {
            Assert_(false);
        }
    }

    //==============================================================================
    float3 EvaluateBsdf(const SurfaceParameters& surface, float3 wo, float3 wi, float& pdf)
    {
        if(surface.shader == eDisney) {
            return CalculateDisneyBsdf(surface, wo, wi, pdf);
        }
        else if(surface.shader == eTransparentGgx) {
            return CalculateTransparentGGXBsdf(surface, wo, wi, pdf);
        }
        else {
            Assert_(false);
        }

        return float3::Zero_;
    }
}