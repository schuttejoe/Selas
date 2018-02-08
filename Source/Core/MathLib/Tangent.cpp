//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/Tangent.h>
#include <MathLib/FloatFuncs.h>
#include <MathLib/FloatStructs.h>

#include <math.h>

namespace Shooty {
    namespace Math {
        //==============================================================================
        void ComputeTangentFrame(const float3& p0, const float3& p1, const float3& p2, const float2& uv0, const float2& uv1, const float2& uv2,
                                 float3& tangent, float3& bitangent) {
            float3 s0 = p1 - p0;
            float3 s1 = p2 - p1;
            float2 uvs0 = uv1 - uv0;
            float2 uvs1 = uv2 - uv0;

            float r = 1.0f / (uvs0.x * uvs1.y - uvs1.x * uvs0.y);

            tangent = float3((uvs1.y * s0.x - uvs0.y * s1.x) * r,
                (uvs1.y * s0.y - uvs0.y * s1.y) * r,
                             (uvs1.y * s0.z - uvs0.y * s1.z) * r);

            bitangent = float3((uvs0.x * s1.x - uvs1.x * s0.x) * r,
                (uvs0.x * s1.y - uvs1.x * s0.y) * r,
                               (uvs0.x * s1.z - uvs1.x * s0.z) * r);
        }

        //==============================================================================
        float4 ComputeFinalTangent(const float3& normal, const float3& intangent, const float3& inbitangent) {
            // -- Gram-Schmidt orthogonalization
            float3 tangent = intangent - Dot(normal, intangent) * normal;
            tangent = Normalize(tangent);

            // -- calculate handedness
            float handedness = Dot(Cross(normal, intangent), inbitangent);
            handedness = (handedness < 0.0f) ? -1.0f : 1.0f;

            return float4(tangent.x, tangent.y, tangent.z, handedness);
        }
    }
}