#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "MathLib/FloatStructs.h"

namespace Selas
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
}