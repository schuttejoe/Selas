#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>

namespace Shooty
{
    struct AxisAlignedBox
    {
        float3 min;
        float3 max;
    };

    void MakeInvalid(AxisAlignedBox* box);
    void IncludePosition(AxisAlignedBox* box, float3 position);
}