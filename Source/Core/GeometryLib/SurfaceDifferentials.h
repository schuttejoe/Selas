#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>

namespace Shooty
{
    struct Ray;

    struct SurfaceDifferentials
    {
        float2 duvdx = float2::Zero_;
        float2 duvdy = float2::Zero_;
        float3 dpdx  = float3::Zero_;
        float3 dpdy  = float3::Zero_;

        float3 dndu  = float3::Zero_;
        float3 dndv  = float3::Zero_;
    };

    void CalculateSurfaceDifferentials(const Ray& ray, float3 n, float3 p, float3 dpdu, float3 dpdv, SurfaceDifferentials& outputs);
    Ray MakeDifferentialRay(float3 rxDirection, float3 ryDirection, float3 p, float3 n, float3 wo, float3 wi, const SurfaceDifferentials& differentials, float3 dndu, float3 dndv, float near, float far);
}