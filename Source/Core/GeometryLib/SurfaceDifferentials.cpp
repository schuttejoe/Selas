//==============================================================================
// Joe Schutte
//==============================================================================

#include <GeometryLib/SurfaceDifferentials.h>
#include <GeometryLib/Ray.h>
#include <MathLib/FloatFuncs.h>

namespace Shooty
{
    //==============================================================================
    void CalculateSurfaceDifferentials(const Ray& ray, float3 n, float3 p, float3 dpdu, float3 dpdv, SurfaceDifferentials& outputs)
    {
        // -- See section 10.1.1 of PBRT 2nd edition

        if(ray.hasDifferentials) {
            float d = Dot(n, p);
            float tx = -(Dot(n, ray.rxOrigin) - d) / Dot(n, ray.rxDirection);
            if(Math::IsInf(tx) || Math::IsNaN(tx))
                goto fail;
            float3 px = ray.rxOrigin + tx * ray.rxDirection;

            float ty = -(Dot(n, ray.ryOrigin) - d) / Dot(n, ray.ryDirection);
            if(Math::IsInf(ty) || Math::IsNaN(ty))
                goto fail;
            float3 py = ray.ryOrigin + ty * ray.ryDirection;

            outputs.dpdx = px - p;
            outputs.dpdy = py - p;

            // Initialize A, Bx, and By matrices for offset computation
            float2x2 A;
            float2 Bx;
            float2 By;

            if(Math::Absf(n.x) > Math::Absf(n.y) && Math::Absf(n.x) > Math::Absf(n.z)) {
                A.r0 = float2(dpdu.y, dpdv.y);
                A.r1 = float2(dpdu.z, dpdv.z);
                Bx = float2(px.y - p.y, px.z - p.z);
                By = float2(py.y - p.y, py.z - p.z);
            }
            else if(Math::Absf(n.y) > Math::Absf(n.z)) {
                A.r0 = float2(dpdu.x, dpdv.x);
                A.r1 = float2(dpdu.z, dpdv.z);
                Bx = float2(px.x - p.x, px.z - p.z);
                By = float2(py.x - p.x, py.z - p.z);
            }
            else {
                A.r0 = float2(dpdu.x, dpdv.x);
                A.r1 = float2(dpdu.y, dpdv.y);
                Bx = float2(px.x - p.x, px.y - p.y);
                By = float2(py.x - p.x, py.y - p.y);
            }

            if(!Matrix2x2::SolveLinearSystem(A, Bx, outputs.duvdx)) {
                outputs.duvdx = float2::Zero_;
            }

            if(!Matrix2x2::SolveLinearSystem(A, By, outputs.duvdy)) {
                outputs.duvdy = float2::Zero_;
            }
        }
        else {
        fail:
            outputs = SurfaceDifferentials();
        }
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