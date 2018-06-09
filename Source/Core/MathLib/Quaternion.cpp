//==============================================================================
// Joe Schutte
//==============================================================================

#include "MathLib/Quaternion.h"
#include "MathLib/Trigonometric.h"
#include "MathLib/FloatFuncs.h"
#include "SystemLib/JsAssert.h"

namespace Selas
{
    namespace Math
    {
        namespace Quaternion
        {
            //==============================================================================
            float4 Identity(void)
            {
                return { 0.0f, 0.0f, 0.0f, 1.0f };
            }

            //==============================================================================
            float4 Create(float radians, const float3& axis)
            {
                float halfAngle = radians * 0.5f;
                float cosine = Cosf(halfAngle);
                float sine = Sinf(halfAngle);

                return { sine * axis.x, sine * axis.y, sine * axis.z, cosine };
            }

            //==============================================================================
            float4 Multiply(const float4& l, const float4& r)
            {
                float4 result;

                result.x = l.w * r.x + l.x * r.w + l.y * r.z - l.z * r.y;
                result.y = l.w * r.y - l.x * r.z + l.y * r.w + l.z * r.x;
                result.z = l.w * r.z + l.x * r.y - l.y * r.x + l.z * r.w;
                result.w = l.w * r.w - l.x * r.x - l.y * r.y - l.z * r.z;

                return result;
            }

            //==============================================================================
            float4 Negate(const float4& q)
            {
                return { -q.x, -q.y, -q.z, -q.w };
            }

            //==============================================================================
            float4 Invert(const float4& q)
            {
                return { -q.x, -q.y, -q.z, q.w };
            }

            //==============================================================================
            float4 Normalize(const float4& q)
            {
                float len = Selas::Length(q);
                Assert_(len > 0.0f);

                float ooLength = 1.0f / len;
                float4 result = { ooLength * q.x, ooLength * q.y, ooLength * q.z, ooLength * q.w };
                return result;
            }

            //==============================================================================
            float Dot(const float4& lhs, const float4& rhs)
            {
                return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z) + (lhs.w * rhs.w);
            }

            //==============================================================================
            float LengthSquared(const float4& q)
            {
                return Selas::Dot(q, q);
            }

            //==============================================================================
            float Length(const float4& q)
            {
                return Sqrtf(Selas::Dot(q, q));
            }

            //==============================================================================
            float3 Rotate(const float4& quat, const float3& v)
            {
                // https://blog.molecular-matters.com/2013/05/24/a-faster-quaternion-vector-multiplication/
                float3 q = { quat.x, quat.y, quat.z };
                float3 t = 2.0f * Selas::Cross(q, v);

                return v + (t * quat.w) + Selas::Cross(q, t);
            }
        }
    }
}