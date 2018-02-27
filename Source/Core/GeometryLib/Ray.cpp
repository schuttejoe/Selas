//==============================================================================
// Joe Schutte
//==============================================================================

#include <GeometryLib/Ray.h>
#include <MathLib/FloatFuncs.h>

namespace Shooty
{
    //==============================================================================
    Ray MakeRay(float3 origin, float3 direction, float near, float far)
    {
        Ray ray;
        ray.origin           = origin;
        ray.direction        = direction;
        ray.rxOrigin         = float3::Zero_;
        ray.rxDirection      = float3::Zero_;
        ray.ryOrigin         = float3::Zero_;
        ray.ryDirection      = float3::Zero_;
        ray.tnear            = near;
        ray.tfar             = far;
        ray.hasDifferentials = false;

        return ray;
    }
}