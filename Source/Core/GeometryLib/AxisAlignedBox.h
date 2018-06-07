#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "MathLib/FloatStructs.h"

namespace Selas
{
    struct AxisAlignedBox
    {
        float3 min;
        float3 max;
    };

    void MakeInvalid(AxisAlignedBox* box);
    void IncludePosition(AxisAlignedBox* box, float3 position);
}