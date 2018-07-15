
//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "Shading/IntegratorContexts.h"
#include "Shading/SurfaceParameters.h"
#include "MathLib/FloatFuncs.h"

namespace Selas
{
    //=============================================================================================================================
    Ray CreateReflectionBounceRay(const SurfaceParameters& surface, const HitParameters& hit, float3 wi, float3 reflectance)
    {
        float3 offsetOrigin = OffsetRayOrigin(surface, wi, 1.0f);
        return MakeRay(offsetOrigin, wi);
    }

    //=============================================================================================================================
    Ray CreateRefractionBounceRay(const SurfaceParameters& surface, const HitParameters& hit, float3 wi, float3 reflectance,
                                  float iorRatio)
    {
        float3 offsetOrigin = OffsetRayOrigin(surface, wi, 1.0f);
        return MakeRay(offsetOrigin, wi);
    }
}