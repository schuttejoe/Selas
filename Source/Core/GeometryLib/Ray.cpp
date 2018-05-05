//==============================================================================
// Joe Schutte
//==============================================================================

#include <GeometryLib/Ray.h>
#include <GeometryLib/SurfaceDifferentials.h>
#include <MathLib/FloatFuncs.h>

namespace Shooty
{
    //==============================================================================
    Ray MakeRay(float3 origin, float3 direction, float near, float far, float ior)
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
        ray.mediumIOR        = ior;

        return ray;
    }

    //==============================================================================
    Ray MakeDifferentialRay(float3 rxDirection, float3 ryDirection, float3 p, float3 n, float3 wo, float3 wi, const SurfaceDifferentials& differentials, float near, float far, float ior)
    {
        // JSTODO - Write this up in the notebook

        Ray ray;
        ray.origin = p;
        ray.direction = wi;
        ray.tnear = near;
        ray.tfar = far;
        ray.mediumIOR = ior;

        ray.rxOrigin = p + differentials.dpdx;
        ray.ryOrigin = p + differentials.dpdy;

        float3 dndx = differentials.dndu * differentials.duvdx.x + differentials.dndv * differentials.duvdx.y;
        float3 dndy = differentials.dndu * differentials.duvdy.x + differentials.dndv * differentials.duvdy.y;

        float3 dwodx = -rxDirection - wo;
        float3 dwody = -ryDirection - wo;
        float dDNdx = Dot(dwodx, n) + Dot(wo, dndx);
        float dDNdy = Dot(dwody, n) + Dot(wo, dndy);

        ray.rxDirection = wi - dwodx + 2.f * (Dot(wo, n) * dndx + dDNdx * n);
        ray.ryDirection = wi - dwody + 2.f * (Dot(wo, n) * dndy + dDNdy * n);

        ray.hasDifferentials = true;

        return ray;
    }
}