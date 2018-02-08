#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>

namespace Shooty {

    namespace Math {

        void ComputeTangentFrame(const float3& p0, const float3& p1, const float3& p2, const float2& uv0, const float2& uv1, const float2& uv2,
                                 float3& tangent, float3& bitangent);
        float4 ComputeFinalTangent(const float3& normal, const float3& intangent, const float3& inbitangent);
    }

}