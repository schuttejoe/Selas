#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>
#include <MathLib/FloatFuncs.h>
#include <MathLib/Trigonometric.h>

namespace Shooty
{
    //==============================================================================
    inline void MakeOrthogonalCoordinateSystem(const float3& v1, float3* v2, float3* v3)
    {
        if(Math::Absf(v1.x) > Math::Absf(v1.y))
            *v2 = float3(-v1.z, 0, v1.x) * (1.0f / Math::Sqrtf(v1.x * v1.x + v1.z * v1.z));
        else
            *v2 = float3(0, v1.z, -v1.y) * (1.0f / Math::Sqrtf(v1.y * v1.y + v1.z * v1.z));
        *v3 = Cross(v1, *v2);
    }
}