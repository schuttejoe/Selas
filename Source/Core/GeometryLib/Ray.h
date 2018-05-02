#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>

namespace Shooty
{
    struct Ray
    {
        float3 origin;
        float3 direction;

        float3 rxOrigin;
        float3 rxDirection;
        float3 ryOrigin;
        float3 ryDirection;

        float tnear;
        float tfar;
        float mediumIOR;
        bool hasDifferentials;
    };

    Ray MakeRay(float3 origin, float3 direction, float near, float far, float ior);
}