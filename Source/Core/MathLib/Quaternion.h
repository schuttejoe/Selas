#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================
#include "MathLib/FloatStructs.h"

namespace Selas
{
    namespace Math
    {
        namespace Quaternion
        {
            float4 Identity(void);
            float4 Create(float radians, const float3& axis);
            float4 Multiply(const float4& l, const float4& r);
            float4 Negate(const float4& q);
            float4 Invert(const float4& q);
            float4 Normalize(const float4& q);

            float Dot(const float4& l, const float4& r);
            float LengthSquared(const float4& q);
            float Length(const float4& q);

            float3 Rotate(const float4& q, const float3& v);
        }
    }
}