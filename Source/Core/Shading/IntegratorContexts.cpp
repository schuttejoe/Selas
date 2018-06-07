
//==============================================================================
// Joe Schutte
//==============================================================================

#include "Shading/IntegratorContexts.h"
#include "Shading/SurfaceParameters.h"
#include "MathLib/FloatFuncs.h"

namespace Selas
{
    //==============================================================================
    Ray CreateReflectionBounceRay(const SurfaceParameters& surface, const HitParameters& hit, float3 wi, float3 reflectance)
    {
        float3 offsetOrigin = OffsetRayOrigin(surface, wi, 1.0f);

        Ray bounceRay;

        #if EnableDifferentials_
            bool rayHasDifferentials = surface.rxDirection.x != 0 || surface.rxDirection.y != 0;
   
            if((surface.materialFlags & ePreserveRayDifferentials) && rayHasDifferentials) {
                bounceRay = MakeReflectionRay(surface.rxDirection, surface.ryDirection, offsetOrigin, surface.geometricNormal, surface.view, wi, surface.differentials, throughput);
            }
            else {
                bounceRay = MakeRay(offsetOrigin, wi);
            }
        #else
            bounceRay = MakeRay(offsetOrigin, wi);
        #endif

        return bounceRay;
    }

    //==============================================================================
    Ray CreateRefractionBounceRay(const SurfaceParameters& surface, const HitParameters& hit, float3 wi, float3 reflectance, float iorRatio)
    {
        float3 offsetOrigin = OffsetRayOrigin(surface, wi, 1.0f);

        Ray bounceRay;

        #if EnableDifferentials_
        bool rayHasDifferentials = surface.rxDirection.x != 0 || surface.rxDirection.y != 0;

        if((surface.materialFlags & ePreserveRayDifferentials) && rayHasDifferentials) {
            bounceRay = MakeRefractionRay(surface.rxDirection, surface.ryDirection, offsetOrigin, surface.geometricNormal, surface.view, wi, surface.differentials, iorRatio);
        }
        else {
            bounceRay = MakeRay(offsetOrigin, wi);
        }
        #else
            bounceRay = MakeRay(offsetOrigin, wi);
        #endif

        return bounceRay;
    }
}