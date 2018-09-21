#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "MathLib/FloatStructs.h"
#include "MathLib/Trigonometric.h"
#include "SystemLib/BasicTypes.h"
#include "SystemLib/JsAssert.h"

namespace Selas
{

    ForceInline_ float2 operator+(float2 lhs, float2 rhs)
    {
        float2 result = { lhs.x + rhs.x, lhs.y + rhs.y };
        return result;
    }

    ForceInline_ float3 operator+(float3 lhs, float3 rhs)
    {
        float3 result = { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z };
        return result;
    }

    ForceInline_ float4 operator+(float4 lhs, float4 rhs)
    {
        float4 result = { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w };
        return result;
    }

    ForceInline_ float2 operator+(float2 lhs, float rhs)
    {
        float2 result = { lhs.x + rhs, lhs.y + rhs };
        return result;
    }

    ForceInline_ float3 operator+(float3 lhs, float rhs)
    {
        float3 result = { lhs.x + rhs, lhs.y + rhs, lhs.z + rhs };
        return result;
    }

    ForceInline_ float4 operator+(float4 lhs, float rhs)
    {
        float4 result = { lhs.x + rhs, lhs.y + rhs, lhs.z + rhs, lhs.w + rhs };
        return result;
    }

    ForceInline_ float2 operator-(float2 lhs, float2 rhs)
    {
        float2 result = { lhs.x - rhs.x, lhs.y - rhs.y };
        return result;
    }

    ForceInline_ float3 operator-(float3 lhs, float3 rhs)
    {
        float3 result = { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z };
        return result;
    }

    ForceInline_ float4 operator-(float4 lhs, float4 rhs)
    {
        float4 result = { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w };
        return result;
    }

    ForceInline_ float2 operator*(float2 lhs, float2 rhs)
    {
        float2 result = { lhs.x * rhs.x, lhs.y * rhs.y };
        return result;
    }

    ForceInline_ float3 operator*(float3 lhs, float3 rhs)
    {
        float3 result = { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z };
        return result;
    }

    ForceInline_ float4 operator*(float4 lhs, float4 rhs)
    {
        float4 result = { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w };
        return result;
    }

    ForceInline_ float2 operator*(float2 lhs, float scale)
    {
        float2 result = { lhs.x * scale, lhs.y * scale };
        return result;
    }

    ForceInline_ float3 operator*(float3 lhs, float scale)
    {
        float3 result = { lhs.x * scale, lhs.y * scale, lhs.z * scale };
        return result;
    }

    ForceInline_ float4 operator*(float4 lhs, float scale)
    {
        float4 result = { lhs.x * scale, lhs.y * scale, lhs.z * scale, lhs.w * scale };
        return result;
    }

    ForceInline_ float2 operator/(float2 lhs, float dividend)
    {
        float2 result = { lhs.x / dividend, lhs.y / dividend };
        return result;
    }

    ForceInline_ float3 operator/(float3 lhs, float dividend)
    {
        float3 result = { lhs.x / dividend, lhs.y / dividend, lhs.z / dividend };
        return result;
    }

    ForceInline_ float4 operator/(float4 lhs, float dividend)
    {
        float4 result = { lhs.x / dividend, lhs.y / dividend, lhs.z / dividend, lhs.w / dividend };
        return result;
    }

    ForceInline_ float2 operator*(float scale, float2 rhs)
    {
        float2 result = { rhs.x * scale, rhs.y * scale };
        return result;
    }

    ForceInline_ float3 operator*(float scale, float3 rhs)
    {
        float3 result = { rhs.x * scale, rhs.y * scale, rhs.z * scale };
        return result;
    }

    ForceInline_ float4 operator*(float scale, float4 rhs)
    {
        float4 result = { rhs.x * scale, rhs.y * scale, rhs.z * scale, rhs.w * scale };
        return result;
    }

    ForceInline_ float2 operator/(float dividend, float2 rhs)
    {
        float2 result = { rhs.x / dividend, rhs.y / dividend };
        return result;
    }

    ForceInline_ float3 operator/(float dividend, float3 rhs)
    {
        float3 result = { rhs.x / dividend, rhs.y / dividend, rhs.z / dividend };
        return result;
    }

    ForceInline_ float4 operator/(float dividend, float4 rhs)
    {
        float4 result = { rhs.x / dividend, rhs.y / dividend, rhs.z / dividend, rhs.w / dividend };
        return result;
    }

    ForceInline_ void operator+=(float2& lhs, float2 rhs)
    {
        lhs.x += rhs.x;
        lhs.y += rhs.y;
    }

    ForceInline_ void operator+=(float3& lhs, float3 rhs)
    {
        lhs.x += rhs.x;
        lhs.y += rhs.y;
        lhs.z += rhs.z;
    }

    ForceInline_ void operator+=(float4& lhs, float4 rhs)
    {
        lhs.x += rhs.x;
        lhs.y += rhs.y;
        lhs.z += rhs.z;
        lhs.w += rhs.w;
    }

    ForceInline_ float2 operator-(float2 lhs)
    {
        float2 result = { -lhs.x, -lhs.y };
        return result;
    }

    ForceInline_ float3 operator-(float3 lhs)
    {
        float3 result = { -lhs.x, -lhs.y, -lhs.z };
        return result;
    }

    ForceInline_ float4 operator-(float4 lhs)
    {
        float4 result = { -lhs.x, -lhs.y, -lhs.z, -lhs.w };
        return result;
    }

    ForceInline_ float3 Cross(float3 lhs, float3 rhs)
    {
        float3 result = {
          lhs.y * rhs.z - lhs.z * rhs.y,
          lhs.z * rhs.x - lhs.x * rhs.z,
          lhs.x * rhs.y - lhs.y * rhs.x,
        };
        return result;
    }

    ForceInline_ float Dot(float2 lhs, float2 rhs)
    {
        return (lhs.x * rhs.x) + (lhs.y * rhs.y);
    }

    ForceInline_ float Dot(float3 lhs, float3 rhs)
    {
        return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
    }

    ForceInline_ float Dot(float4 lhs, float4 rhs)
    {
        return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z) + (lhs.w * rhs.w);
    }

    ForceInline_ float LengthSquared(float2 vec)
    {
        return Dot(vec, vec);
    }

    ForceInline_ float LengthSquared(float3 vec)
    {
        return Dot(vec, vec);
    }

    ForceInline_ float LengthSquared(float4 vec)
    {
        return Dot(vec, vec);
    }

    ForceInline_ float Length(float2 vec)
    {
        return Math::Sqrtf(Dot(vec, vec));
    }

    ForceInline_ float Length(float3 vec)
    {
        return Math::Sqrtf(Dot(vec, vec));
    }

    ForceInline_ float Length(float4 vec)
    {
        return Math::Sqrtf(Dot(vec, vec));
    }

    ForceInline_ float LengthInverse(float3 vec3)
    {
        return 1.f / Length(vec3);
    }

    ForceInline_ float LengthInverse(float4 vec4)
    {
        return 1.f / Length(vec4);
    }

    ForceInline_ float2 Pow(float base, float2 vec)
    {
        return float2(Math::Powf(base, vec.x), Math::Powf(base, vec.y));
    }

    ForceInline_ float3 Pow(float base, float3 vec)
    {
        return float3(Math::Powf(base, vec.x), Math::Powf(base, vec.y), Math::Powf(base, vec.z));
    }

    ForceInline_ float4 Pow(float base, float4 vec)
    {
        return float4(Math::Powf(base, vec.x), Math::Powf(base, vec.y), Math::Powf(base, vec.z), Math::Powf(base, vec.w));
    }

    ForceInline_ float2 Pow(float2 vec, float exponent)
    {
        return float2(Math::Powf(vec.x, exponent), Math::Powf(vec.y, exponent));
    }

    ForceInline_ float3 Pow(float3 vec, float exponent)
    {
        return float3(Math::Powf(vec.x, exponent), Math::Powf(vec.y, exponent), Math::Powf(vec.z, exponent));
    }

    ForceInline_ float4 Pow(float4 vec, float exponent)
    {
        return float4(Math::Powf(vec.x, exponent), Math::Powf(vec.y, exponent),
                      Math::Powf(vec.z, exponent), Math::Powf(vec.w, exponent));
    }

    ForceInline_ float2 Exp(float2 vec)
    {
        return float2(Math::Expf(vec.x), Math::Expf(vec.y));
    }

    ForceInline_ float3 Exp(float3 vec)
    {
        return float3(Math::Expf(vec.x), Math::Expf(vec.y), Math::Expf(vec.z));
    }

    ForceInline_ float4 Exp(float4 vec)
    {
        return float4(Math::Expf(vec.x), Math::Expf(vec.y), Math::Expf(vec.z), Math::Expf(vec.w));
    }

    ForceInline_ float2 Sqrt(float2 vec)
    {
        return float2(Math::Sqrtf(vec.x), Math::Sqrtf(vec.y));
    }

    ForceInline_ float3 Sqrt(float3 vec)
    {
        return float3(Math::Sqrtf(vec.x), Math::Sqrtf(vec.y), Math::Sqrtf(vec.z));
    }

    ForceInline_ float4 Sqrt(float4 vec)
    {
        return float4(Math::Sqrtf(vec.x), Math::Sqrtf(vec.y), Math::Sqrtf(vec.z), Math::Sqrtf(vec.w));
    }

    ForceInline_ float3 ProjectOntoV(float3 u, float3 v)
    {
        float d = Dot(u, v);
        float v2 = Dot(v, v);

        return (d / v2) * v;
    }

    // -- both should face outward
    ForceInline_ float3 Reflect(float3 n, float3 l)
    {
        return 2.0f * Dot(n, l) * n - l;
    }

    ForceInline_ bool Transmit(float3 wm, float3 wi, float n, float3& wo)
    {
        float c = Dot(wi, wm);
        if(c < 0.0f) {
            c = -c;
            wm = -wm;
        }

        float root = 1.0f - n * n * (1.0f - c * c);
        if(root <= 0)
            return false;

        wo = (n * c - Math::Sqrtf(root)) * wm - n * wi;
        return true;
    }

    ForceInline_ float Lerp(float a, float b, float t)
    {
        return (1.0f - t) * a + t * b;
    }

    ForceInline_ float2 Lerp(float2 a, float2 b, float t)
    {
        return (1.0f - t) * a + t * b;
    }

    ForceInline_ float3 Lerp(float3 a, float3 b, float t)
    {
        return (1.0f - t) * a + t * b;
    }

    ForceInline_ float4 Lerp(float4 a, float4 b, float t)
    {
        return (1.0f - t) * a + t * b;
    }

    ForceInline_ float Saturate(float x)
    {
        if(x < 0.0f) {
            return 0.0f;
        }
        else if(x > 1.0f) {
            return 1.0f;
        }

        return x;
    }

    ForceInline_ float3 Normalize(float3 vec3)
    {
        float invLength = LengthInverse(vec3);

        float3 result = { vec3.x * invLength, vec3.y * invLength, vec3.z * invLength };
        return result;
    }

    ForceInline_ float4 Normalize(float4 vec4)
    {
        float invLength = LengthInverse(vec4);

        float4 result = { vec4.x * invLength, vec4.y * invLength, vec4.z * invLength, vec4.w * invLength };
        return result;
    }

    namespace Matrix4x4
    {
        float4x4 Identity(void);
        float4x4 Zero(void);
        float4x4 Scale(float x, float y, float z);
        float4x4 Translate(float x, float y, float z);
        float4x4 ScaleTranslate(float s, float tx, float ty, float tz);
        float4x4 ScaleTranslate(float sx, float sy, float sz, float tx, float ty, float tz);
        float4x4 RotateX(float radians);
        float4x4 RotateY(float radians);
        float4x4 RotateZ(float radians);
    };

    namespace Matrix3x3
    {
        float3x3 Identity(void);
    };

    namespace Matrix2x2
    {
        bool SolveLinearSystem(float2x2 A, float2 B, float2& r);
    }

    // 3x3 matrix functions
    float3x3 MatrixTranspose(const float3x3& mat);

    // 4x4 matrix functions
    float4x4 MatrixTranspose(const float4x4& mat);
    float4x4 MatrixInverse(const float4x4& mat);
    float4x4 MatrixMultiply(const float4x4& lhs, const float4x4& rhs);
    float3   MatrixMultiply(const float3& vec, const float3x3& mat);
    float3   MatrixMultiplyVector(const float3& vec, const float4x4& mat);
    float3   MatrixMultiplyPoint(const float3& vec, const float4x4& mat);
    float4   MatrixMultiplyFloat4(const float4& vec, const float4x4& mat);

    float4x4 ScreenProjection(uint width, uint height);
    float4x4 ScreenProjection(float x, float y, uint width, uint height);
    float4x4 PerspectiveFovLhProjection(float fov, float aspect, float near, float far);
    float4x4 OffsetCenterProjectionLh(float min_x, float max_x, float min_y, float max_y, float min_z, float max_z);
    float4x4 LookAtLh(float3 eye, float3 up, float3 target);
    float4x4 ViewLh(float3 position, float3 forward, float3 up, float3 right);

    float4x4 PerspectiveFovRhProjection(float fov, float aspect, float near, float far);
    float4x4 LookAtRh(float3 eye, float3 up, float3 target);
}