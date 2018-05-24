#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>

namespace Selas
{
    struct SurfaceDifferentials;

    struct Ray
    {
        float3 origin;
        float3 direction;

        float3 rxOrigin;
        float3 rxDirection;
        float3 ryOrigin;
        float3 ryDirection;

        uint32 bounceCount;
        float3 throughput;
        uint32 pixelIndex;
    };

    Ray MakeRay(float3 origin, float3 direction, float3 throughput, uint32 pixelIndex, uint32 bounceCount);
    Ray MakeReflectionRay(float3 rxDirection, float3 ryDirection, float3 p, float3 n, float3 wo, float3 wi, const SurfaceDifferentials& differentials, float3 throughput, uint32 pixelIndex, uint32 bounceCount);
    Ray MakeRefractionRay(float3 rxDirection, float3 ryDirection, float3 p, float3 n, float3 wo, float3 wi, const SurfaceDifferentials& differentials, float iorRatio, float3 throughput, uint32 pixelIndex, uint32 bounceCount);
}