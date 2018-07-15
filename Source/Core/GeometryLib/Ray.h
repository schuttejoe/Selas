#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "MathLib/FloatStructs.h"

namespace Selas
{
    struct SurfaceDifferentials;

    struct Ray
    {
        float3 origin;
        float3 direction;
    };

    Ray MakeRay(float3 origin, float3 direction);
}