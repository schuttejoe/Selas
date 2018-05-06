//==============================================================================
// Joe Schutte
//==============================================================================

#include <GeometryLib/Ray.h>
#include <GeometryLib/SurfaceDifferentials.h>
#include <MathLib/FloatFuncs.h>

namespace Shooty
{
    //==============================================================================
    Ray MakeRay(float3 origin, float3 direction, float3 throughput, uint32 pixelIndex)
    {
        Ray ray;
        ray.origin      = origin;
        ray.direction   = direction;
        ray.rxOrigin    = float3::Zero_;
        ray.rxDirection = float3::Zero_;
        ray.ryOrigin    = float3::Zero_;
        ray.ryDirection = float3::Zero_;
        ray.throughput  = throughput;
        ray.pixelIndex  = pixelIndex;
        ray.bounceCount = 0;

        return ray;
    }

    //==============================================================================
    Ray MakeDifferentialRay(float3 rxDirection, float3 ryDirection, float3 p, float3 n, float3 wo, float3 wi, const SurfaceDifferentials& differentials, float3 throughput, uint32 pixelIndex)
    {
        // JSTODO - Write this up in the notebook

        Ray ray;
        ray.origin      = p;
        ray.direction   = wi;
        ray.throughput  = throughput;
        ray.pixelIndex  = pixelIndex;
        ray.bounceCount = 0;

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

        return ray;
    }
}