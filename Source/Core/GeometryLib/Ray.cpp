//==============================================================================
// Joe Schutte
//==============================================================================

#include "GeometryLib/Ray.h"
#include "GeometryLib/SurfaceDifferentials.h"
#include "MathLib/FloatFuncs.h"

namespace Selas
{
    //==============================================================================
    Ray MakeRay(float3 origin, float3 direction)
    {
        Ray ray;
        ray.origin      = origin;
        ray.direction   = direction;
        ray.rxOrigin    = float3::Zero_;
        ray.rxDirection = float3::Zero_;
        ray.ryOrigin    = float3::Zero_;
        ray.ryDirection = float3::Zero_;

        return ray;
    }

    //==============================================================================
    // -- See Tracing Ray Differentials [Igehy 1999] - https://graphics.stanford.edu/papers/trd/
    Ray MakeReflectionRay(float3 rxDirection, float3 ryDirection, float3 p, float3 n, float3 wo, float3 wi, const SurfaceDifferentials& differentials)
    {
        Ray ray;
        ray.origin      = p;
        ray.direction   = wi;
        ray.rxOrigin    = p + differentials.dpdx;
        ray.ryOrigin    = p + differentials.dpdy;

        float3 dndx = differentials.dndu * differentials.duvdx.x + differentials.dndv * differentials.duvdx.y;
        float3 dndy = differentials.dndu * differentials.duvdy.x + differentials.dndv * differentials.duvdy.y;

        float3 dwodx = -rxDirection - wo;
        float3 dwody = -ryDirection - wo;
        float dDNdx = Dot(dwodx, n) + Dot(wo, dndx);
        float dDNdy = Dot(dwody, n) + Dot(wo, dndy);

        ray.rxDirection = wi - dwodx + 2.f * (Dot(wo, n) * dndx + dDNdx * n);
        ray.ryDirection = wi - dwody + 2.f * (Dot(wo, n) * dndy + dDNdy * n);

        return ray;
    }

    //==============================================================================
    // -- See Tracing Ray Differentials [Igehy 1999] - https://graphics.stanford.edu/papers/trd/
    Ray MakeRefractionRay(float3 rxDirection, float3 ryDirection, float3 p, float3 n, float3 wo, float3 wi, const SurfaceDifferentials& differentials, float iorRatio)
    {
        Ray ray;
        ray.origin      = p;
        ray.direction   = wi;
        ray.rxOrigin    = p + differentials.dpdx;
        ray.ryOrigin    = p + differentials.dpdy;

        float3 dndx = differentials.dndu * differentials.duvdx.x + differentials.dndv * differentials.duvdx.y;
        float3 dndy = differentials.dndu * differentials.duvdy.x + differentials.dndv * differentials.duvdy.y;

        float3 dwodx = -rxDirection - wo;
        float3 dwody = -ryDirection - wo;
        float dDNdx = Dot(dwodx, n) + Dot(wo, dndx);
        float dDNdy = Dot(dwody, n) + Dot(wo, dndy);

        // -- ray direction for the incoming ray
        float3 D = -wo;

        float mu = iorRatio * Dot(D, n) - Dot(wi, n);
        float dmudx = (iorRatio - (iorRatio * iorRatio * Dot(D, n)) / Dot(wi, n)) * dDNdx;
        float dmudy = (iorRatio - (iorRatio * iorRatio * Dot(D, n)) / Dot(wi, n)) * dDNdy;

        ray.rxDirection = wi + iorRatio * dwodx - (mu * dndx + dmudx * n);
        ray.ryDirection = wi + iorRatio * dwody - (mu * dndy + dmudy * n);

        return ray;
    }
}