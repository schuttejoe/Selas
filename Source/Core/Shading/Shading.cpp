
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
    Ray CreateReflectionBounceRay(const SurfaceParameters& surface, const HitParameters& hit, float3 wi, float3 reflectance)
    {
        float3 offsetOrigin = OffsetRayOrigin(surface, wi, 1.0f);
        float3 throughput = hit.throughput * reflectance;

        bool rayHasDifferentials = surface.rxDirection.x != 0 || surface.rxDirection.y != 0;

        Ray bounceRay;
        if((surface.materialFlags & ePreserveRayDifferentials) && rayHasDifferentials) {
            bounceRay = MakeReflectionRay(surface.rxDirection, surface.ryDirection, offsetOrigin, surface.perturbedNormal, hit.viewDirection, wi, surface.differentials, throughput, hit.pixelIndex, hit.bounceCount + 1);
        }
        else {
            bounceRay = MakeRay(offsetOrigin, wi, throughput, hit.pixelIndex, hit.bounceCount + 1);
        }

        return bounceRay;
    }

    //==============================================================================
    Ray CreateRefractionBounceRay(const SurfaceParameters& surface, const HitParameters& hit, float3 wi, float3 reflectance, float iorRatio)
    {
        float3 offsetOrigin = OffsetRayOrigin(surface, wi, 1.0f);
        float3 throughput = hit.throughput * reflectance;

        bool rayHasDifferentials = surface.rxDirection.x != 0 || surface.rxDirection.y != 0;

        Ray bounceRay;
        if((surface.materialFlags & ePreserveRayDifferentials) && rayHasDifferentials) {
            bounceRay = MakeRefractionRay(surface.rxDirection, surface.ryDirection, offsetOrigin, surface.perturbedNormal, hit.viewDirection, wi, surface.differentials, iorRatio, throughput, hit.pixelIndex, hit.bounceCount + 1);
        }
        else {
            bounceRay = MakeRay(offsetOrigin, wi, throughput, hit.pixelIndex, hit.bounceCount + 1);
        }

        return bounceRay;
    }

    //==============================================================================
    void InsertRay(KernelContext* context, const Ray& ray)
    {
        if(ray.bounceCount == context->maxPathLength)
            return;

        Assert_(context->rayStackCount + 1 != context->rayStackCapacity);
        context->rayStack[context->rayStackCount] = ray;
        ++context->rayStackCount;
    }

    //==============================================================================
    void AccumulatePixelEnergy(KernelContext* context, const Ray& ray, float3 value)
    {
        context->imageData[ray.pixelIndex] += ray.throughput * value;
    }

    //==============================================================================
    void AccumulatePixelEnergy(KernelContext* context, const HitParameters& hit, float3 value)
    {
        context->imageData[hit.pixelIndex] += hit.throughput * value;
    }

    //==============================================================================
    void ShadeSurfaceHit(KernelContext* context, const HitParameters& hit)
    {
        SurfaceParameters surface;
        if(CalculateSurfaceParams(context, &hit, surface) == false) {
            return;
        }

        if(surface.shader == eDisney) {
            DisneyBrdfShader(context, hit, surface);
        }
        else if(surface.shader == eTransparentGgx) {
            TransparentGgxShader(context, hit, surface);
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