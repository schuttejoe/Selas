#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "MathLib/FloatStructs.h"

namespace Selas
{
    class CSerializer;

    struct AxisAlignedBox
    {
        float3 min;
        float3 max;
    };

    void Serialize(CSerializer* serializer, AxisAlignedBox& v);

    void MakeInvalid(AxisAlignedBox* box);
    void IncludePosition(AxisAlignedBox* box, float3 position);
    void IncludeBox(AxisAlignedBox* box, const AxisAlignedBox& right);
    void IncludeBox(AxisAlignedBox* box, float4x4 transform, const AxisAlignedBox& right);
}