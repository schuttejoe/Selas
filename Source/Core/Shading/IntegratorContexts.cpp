
//==============================================================================
// Joe Schutte
//==============================================================================

#include <Shading/IntegratorContexts.h>
#include <Shading/SurfaceParameters.h>
#include <MathLib/FloatFuncs.h>

namespace Selas
{
    //==============================================================================
    Ray CreateReflectionBounceRay(const SurfaceParameters& surface, const HitParameters& hit, float3 wi, float3 reflectance)
    {
        float3 offsetOrigin = OffsetRayOrigin(surface, wi, 1.0f);
        float3 throughput = hit.throughput * reflectance;

        Ray bounceRay;

        #if EnableDifferentials_
            bool rayHasDifferentials = surface.rxDirection.x != 0 || surface.rxDirection.y != 0;
   
            if((surface.materialFlags & ePreserveRayDifferentials) && rayHasDifferentials) {
                bounceRay = MakeReflectionRay(surface.rxDirection, surface.ryDirection, offsetOrigin, surface.geometricNormal, hit.viewDirection, wi, surface.differentials, throughput, hit.pixelIndex, hit.bounceCount + 1);
            }
            else {
                bounceRay = MakeRay(offsetOrigin, wi, throughput, hit.pixelIndex, hit.bounceCount + 1);
            }
        #else
            bounceRay = MakeRay(offsetOrigin, wi, throughput, hit.pixelIndex, hit.bounceCount + 1);
        #endif

        return bounceRay;
    }

    //==============================================================================
    Ray CreateRefractionBounceRay(const SurfaceParameters& surface, const HitParameters& hit, float3 wi, float3 reflectance, float iorRatio)
    {
        float3 offsetOrigin = OffsetRayOrigin(surface, wi, 1.0f);
        float3 throughput = hit.throughput * reflectance;

        Ray bounceRay;

        #if EnableDifferentials_
        bool rayHasDifferentials = surface.rxDirection.x != 0 || surface.rxDirection.y != 0;

        if((surface.materialFlags & ePreserveRayDifferentials) && rayHasDifferentials) {
            bounceRay = MakeRefractionRay(surface.rxDirection, surface.ryDirection, offsetOrigin, surface.geometricNormal, hit.viewDirection, wi, surface.differentials, iorRatio, throughput, hit.pixelIndex, hit.bounceCount + 1);
        }
        else {
            bounceRay = MakeRay(offsetOrigin, wi, throughput, hit.pixelIndex, hit.bounceCount + 1);
        }
        #else
            bounceRay = MakeRay(offsetOrigin, wi, throughput, hit.pixelIndex, hit.bounceCount + 1);
        #endif

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
}