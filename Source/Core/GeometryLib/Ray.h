#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "MathLib/FloatStructs.h"

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
    };

    Ray MakeRay(float3 origin, float3 direction);
    Ray MakeReflectionRay(float3 rxDirection, float3 ryDirection, float3 p, float3 n, float3 wo, float3 wi, const SurfaceDifferentials& differentials);
    Ray MakeRefractionRay(float3 rxDirection, float3 ryDirection, float3 p, float3 n, float3 wo, float3 wi, const SurfaceDifferentials& differentials, float iorRatio);
}