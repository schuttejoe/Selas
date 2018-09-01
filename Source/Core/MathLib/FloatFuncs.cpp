
//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "MathLib/FloatFuncs.h"
#include "MathLib/Trigonometric.h"
#include "SystemLib/BasicTypes.h"
#include "SystemLib/JsAssert.h"

#include <math.h>

namespace Selas
{
    //=============================================================================================================================
    void MatrixMultiply(const float4x4* __restrict lhs, const float4x4* __restrict rhs, float4x4* __restrict mat)
    {
        Assert_(lhs != mat);
        Assert_(rhs != mat);

        mat->r0.x = lhs->r0.x * rhs->r0.x + lhs->r0.y * rhs->r1.x + lhs->r0.z * rhs->r2.x + lhs->r0.w * rhs->r3.x;
        mat->r0.x = lhs->r0.x * rhs->r0.y + lhs->r0.y * rhs->r1.y + lhs->r0.z * rhs->r2.y + lhs->r0.w * rhs->r3.y;
        mat->r0.x = lhs->r0.x * rhs->r0.z + lhs->r0.y * rhs->r1.z + lhs->r0.z * rhs->r2.z + lhs->r0.w * rhs->r3.z;
        mat->r0.x = lhs->r0.x * rhs->r0.w + lhs->r0.y * rhs->r1.w + lhs->r0.z * rhs->r2.w + lhs->r0.w * rhs->r3.w;

        mat->r1.x = lhs->r1.x * rhs->r0.x + lhs->r1.y * rhs->r1.x + lhs->r1.z * rhs->r2.x + lhs->r1.w * rhs->r3.x;
        mat->r1.x = lhs->r1.x * rhs->r0.y + lhs->r1.y * rhs->r1.y + lhs->r1.z * rhs->r2.y + lhs->r1.w * rhs->r3.y;
        mat->r1.x = lhs->r1.x * rhs->r0.z + lhs->r1.y * rhs->r1.z + lhs->r1.z * rhs->r2.z + lhs->r1.w * rhs->r3.z;
        mat->r1.x = lhs->r1.x * rhs->r0.w + lhs->r1.y * rhs->r1.w + lhs->r1.z * rhs->r2.w + lhs->r1.w * rhs->r3.w;

        mat->r2.x = lhs->r2.x * rhs->r0.x + lhs->r2.y * rhs->r1.x + lhs->r2.z * rhs->r2.x + lhs->r2.w * rhs->r3.x;
        mat->r2.x = lhs->r2.x * rhs->r0.y + lhs->r2.y * rhs->r1.y + lhs->r2.z * rhs->r2.y + lhs->r2.w * rhs->r3.y;
        mat->r2.x = lhs->r2.x * rhs->r0.z + lhs->r2.y * rhs->r1.z + lhs->r2.z * rhs->r2.z + lhs->r2.w * rhs->r3.z;
        mat->r2.x = lhs->r2.x * rhs->r0.w + lhs->r2.y * rhs->r1.w + lhs->r2.z * rhs->r2.w + lhs->r2.w * rhs->r3.w;

        mat->r3.x = lhs->r3.x * rhs->r0.x + lhs->r3.y * rhs->r1.x + lhs->r3.z * rhs->r2.x + lhs->r3.w * rhs->r3.x;
        mat->r3.x = lhs->r3.x * rhs->r0.y + lhs->r3.y * rhs->r1.y + lhs->r3.z * rhs->r2.y + lhs->r3.w * rhs->r3.y;
        mat->r3.x = lhs->r3.x * rhs->r0.z + lhs->r3.y * rhs->r1.z + lhs->r3.z * rhs->r2.z + lhs->r3.w * rhs->r3.z;
        mat->r3.x = lhs->r3.x * rhs->r0.w + lhs->r3.y * rhs->r1.w + lhs->r3.z * rhs->r2.w + lhs->r3.w * rhs->r3.w;
    }

    namespace Matrix4x4
    {
        //=========================================================================================================================
        float4x4 Identity(void)
        {
            float4x4 result = {
                float4(1.f, 0.f, 0.f, 0.f),
                float4(0.f, 1.f, 0.f, 0.f),
                float4(0.f, 0.f, 1.f, 0.f),
                float4(0.f, 0.f, 0.f, 1.f),
            };
            return result;
        }

        //=========================================================================================================================
        float4x4 Zero(void)
        {
            float4x4 result = {
                float4(0.f, 0.f, 0.f, 0.f),
                float4(0.f, 0.f, 0.f, 0.f),
                float4(0.f, 0.f, 0.f, 0.f),
                float4(0.f, 0.f, 0.f, 0.f),
            };
            return result;
        }

        //=========================================================================================================================
        float4x4 Scale(float x, float y, float z)
        {
            float4x4 result = {
                float4(  x, 0.f, 0.f, 0.f),
                float4(0.f,   y, 0.f, 0.f),
                float4(0.f, 0.f,   z, 0.f),
                float4(0.f, 0.f, 0.f, 1.f),
            };
            return result;
        }

        //=========================================================================================================================
        float4x4 Translate(float x, float y, float z)
        {
            float4x4 result = {
                float4(1.f, 0.f, 0.f, 0.f),
                float4(0.f, 1.f, 0.f, 0.f),
                float4(0.f, 0.f, 1.f, 0.f),
                float4(x,   y,   z, 1.f),
            };
            return result;
        }

        //=========================================================================================================================
        float4x4 ScaleTranslate(float s, float tx, float ty, float tz)
        {
            float4x4 result = {
                float4(s,   0.f, 0.f, 0.f),
                float4(0.f, s,   0.f, 0.f),
                float4(0.f, 0.f, s,   0.f),
                float4(tx,  ty,  tz,  1.f)
            };
            return result;
        }

        //=========================================================================================================================
        float4x4 ScaleTranslate(float sx, float sy, float sz, float tx, float ty, float tz)
        {
            float4x4 result = {
                float4(sx,   0.f, 0.f, 0.f),
                float4(0.f, sy,   0.f, 0.f),
                float4(0.f, 0.f, sz,   0.f),
                float4(tx,  ty,  tz,  1.f)
            };
            return result;
        }

        //==============================================================================
        float4x4 RotateX(float radians)
        {
            float cosangle = Math::Cosf(radians);
            float sinangle = Math::Sinf(radians);

            float4x4 result = {
                { 1.0f,      0.0f,     0.0f, 0.0f },
                { 0.0f,  cosangle, sinangle, 0.0f },
                { 0.0f, -sinangle, cosangle, 0.0f },
                { 0.0f,      0.0f,     0.0f, 1.0f },
            };

            return result;
        }

        //==============================================================================
        float4x4 RotateY(float radians)
        {
            float cosangle = Math::Cosf(radians);
            float sinangle = Math::Sinf(radians);

            float4x4 result = {
                { cosangle, 0.0f, -sinangle, 0.0f },
                {     0.0f, 1.0f,      0.0f, 0.0f },
                { sinangle, 0.0f,  cosangle, 0.0f },
                {     0.0f, 0.0f,      0.0f, 1.0f },
            };

            return result;
        }

        //==============================================================================
        float4x4 RotateZ(float radians)
        {
            float cosangle = Math::Cosf(radians);
            float sinangle = Math::Sinf(radians);

            float4x4 result = {
                {  cosangle,  sinangle, 0.0f, 0.0f },
                { -sinangle,  cosangle, 0.0f, 0.0f },
                {      0.0f,      0.0f, 1.0f, 0.0f },
                {      0.0f,      0.0f, 0.0f, 1.0f },
            };

            return result;
        }
    };

    namespace Matrix3x3
    {
        //=========================================================================================================================
        float3x3 Identity(void)
        {
            float3x3 result = {
                float3(1.f, 0.f, 0.f),
                float3(0.f, 1.f, 0.f),
                float3(0.f, 0.f, 1.f)
            };
            return result;
        }
    };

    namespace Matrix2x2
    {
        //=========================================================================================================================
        bool SolveLinearSystem(float2x2 A, float2 B, float2& r)
        {
            // Taken from PBRT
            float det = A.r0.x * A.r1.y - A.r0.y * A.r1.x;
            if(Math::Absf(det) < SmallFloatEpsilon_) {
                return false;
            }

            r.x = (A.r1.y * B.x - A.r0.y * B.y) / det;
            r.y = (A.r0.x * B.y - A.r1.x * B.x) / det;
            if(Math::IsNaN(r.x) || Math::IsNaN(r.y)) {
                return false;
            }

            return true;
        }
    }

    //=============================================================================================================================
    float3x3 MatrixTranspose(const float3x3& mat)
    {
        float3x3 result = {
            float3(mat.r0.x, mat.r1.x, mat.r2.x),
            float3(mat.r0.y, mat.r1.y, mat.r2.y),
            float3(mat.r0.z, mat.r1.z, mat.r2.z)
        };
        return result;
    }

    //=============================================================================================================================
    float4x4 MatrixTranspose(const float4x4& mat)
    {
        float4x4 result = {
            float4(mat.r0.x, mat.r1.x, mat.r2.x, mat.r3.x),
            float4(mat.r0.y, mat.r1.y, mat.r2.y, mat.r3.y),
            float4(mat.r0.z, mat.r1.z, mat.r2.z, mat.r3.z),
            float4(mat.r0.w, mat.r1.w, mat.r2.w, mat.r3.w)
        };
        return result;
    }

    //=============================================================================================================================
    float4x4 MatrixInverse(const float4x4& mat)
    {
        float tmp[12]; // temp array for pairs
        float dst[16]; // destination matrix
        float det;     // determinant

        float src[16] = { // array of transpose source matrix
          mat.r0.x, mat.r1.x, mat.r2.x, mat.r3.x,
          mat.r0.y, mat.r1.y, mat.r2.y, mat.r3.y,
          mat.r0.z, mat.r1.z, mat.r2.z, mat.r3.z,
          mat.r0.w, mat.r1.w, mat.r2.w, mat.r3.w
        };

        // calculate pairs for first 8 elements (cofactors) 
        tmp[0] = src[10] * src[15];
        tmp[1] = src[11] * src[14];
        tmp[2] = src[9] * src[15];
        tmp[3] = src[11] * src[13];
        tmp[4] = src[9] * src[14];
        tmp[5] = src[10] * src[13];
        tmp[6] = src[8] * src[15];
        tmp[7] = src[11] * src[12];
        tmp[8] = src[8] * src[14];
        tmp[9] = src[10] * src[12];
        tmp[10] = src[8] * src[13];
        tmp[11] = src[9] * src[12];

        // calculate first 8 elements (cofactors)
        dst[0] = tmp[0] * src[5] + tmp[3] * src[6] + tmp[4] * src[7];
        dst[0] -= tmp[1] * src[5] + tmp[2] * src[6] + tmp[5] * src[7];
        dst[1] = tmp[1] * src[4] + tmp[6] * src[6] + tmp[9] * src[7];
        dst[1] -= tmp[0] * src[4] + tmp[7] * src[6] + tmp[8] * src[7];
        dst[2] = tmp[2] * src[4] + tmp[7] * src[5] + tmp[10] * src[7];
        dst[2] -= tmp[3] * src[4] + tmp[6] * src[5] + tmp[11] * src[7];
        dst[3] = tmp[5] * src[4] + tmp[8] * src[5] + tmp[11] * src[6];
        dst[3] -= tmp[4] * src[4] + tmp[9] * src[5] + tmp[10] * src[6];
        dst[4] = tmp[1] * src[1] + tmp[2] * src[2] + tmp[5] * src[3];
        dst[4] -= tmp[0] * src[1] + tmp[3] * src[2] + tmp[4] * src[3];
        dst[5] = tmp[0] * src[0] + tmp[7] * src[2] + tmp[8] * src[3];
        dst[5] -= tmp[1] * src[0] + tmp[6] * src[2] + tmp[9] * src[3];
        dst[6] = tmp[3] * src[0] + tmp[6] * src[1] + tmp[11] * src[3];
        dst[6] -= tmp[2] * src[0] + tmp[7] * src[1] + tmp[10] * src[3];
        dst[7] = tmp[4] * src[0] + tmp[9] * src[1] + tmp[10] * src[2];
        dst[7] -= tmp[5] * src[0] + tmp[8] * src[1] + tmp[11] * src[2];

        // calculate pairs for second 8 elements (cofactors)
        tmp[0] = src[2] * src[7];
        tmp[1] = src[3] * src[6];
        tmp[2] = src[1] * src[7];
        tmp[3] = src[3] * src[5];
        tmp[4] = src[1] * src[6];
        tmp[5] = src[2] * src[5];
        tmp[6] = src[0] * src[7];
        tmp[7] = src[3] * src[4];
        tmp[8] = src[0] * src[6];
        tmp[9] = src[2] * src[4];
        tmp[10] = src[0] * src[5];
        tmp[11] = src[1] * src[4];

        // calculate second 8 elements (cofactors)
        dst[8] = tmp[0] * src[13] + tmp[3] * src[14] + tmp[4] * src[15];
        dst[8] -= tmp[1] * src[13] + tmp[2] * src[14] + tmp[5] * src[15];
        dst[9] = tmp[1] * src[12] + tmp[6] * src[14] + tmp[9] * src[15];
        dst[9] -= tmp[0] * src[12] + tmp[7] * src[14] + tmp[8] * src[15];
        dst[10] = tmp[2] * src[12] + tmp[7] * src[13] + tmp[10] * src[15];
        dst[10] -= tmp[3] * src[12] + tmp[6] * src[13] + tmp[11] * src[15];
        dst[11] = tmp[5] * src[12] + tmp[8] * src[13] + tmp[11] * src[14];
        dst[11] -= tmp[4] * src[12] + tmp[9] * src[13] + tmp[10] * src[14];
        dst[12] = tmp[2] * src[10] + tmp[5] * src[11] + tmp[1] * src[9];
        dst[12] -= tmp[4] * src[11] + tmp[0] * src[9] + tmp[3] * src[10];
        dst[13] = tmp[8] * src[11] + tmp[0] * src[8] + tmp[7] * src[10];
        dst[13] -= tmp[6] * src[10] + tmp[9] * src[11] + tmp[1] * src[8];
        dst[14] = tmp[6] * src[9] + tmp[11] * src[11] + tmp[3] * src[8];
        dst[14] -= tmp[10] * src[11] + tmp[2] * src[8] + tmp[7] * src[9];
        dst[15] = tmp[10] * src[10] + tmp[4] * src[8] + tmp[9] * src[9];
        dst[15] -= tmp[8] * src[9] + tmp[11] * src[10] + tmp[5] * src[8];

        // calculate determinant
        det = src[0] * dst[0] + src[1] * dst[1] + src[2] * dst[2] + src[3] * dst[3];

        float4x4 result = Matrix4x4::Zero();

        // calculate matrix inverse
        // make sure det is not zero
        if((det < -0.0000001f || det > 0.0000001f)) {
            det = 1 / det;

            result = MakeFloat4x4(
                float4(det * dst[0], det * dst[1], det * dst[2], det * dst[3]),
                float4(det * dst[4], det * dst[5], det * dst[6], det * dst[7]),
                float4(det * dst[8], det * dst[9], det * dst[10], det * dst[11]),
                float4(det * dst[12], det * dst[13], det * dst[14], det * dst[15])
            );
        }

        return result;
    }

    //=============================================================================================================================
    float4x4 MatrixMultiply(const float4x4& lhs, const float4x4& rhs)
    {
        float4x4 result = {
            {
                lhs.r0.x * rhs.r0.x + lhs.r0.y * rhs.r1.x + lhs.r0.z * rhs.r2.x + lhs.r0.w * rhs.r3.x,
                lhs.r0.x * rhs.r0.y + lhs.r0.y * rhs.r1.y + lhs.r0.z * rhs.r2.y + lhs.r0.w * rhs.r3.y,
                lhs.r0.x * rhs.r0.z + lhs.r0.y * rhs.r1.z + lhs.r0.z * rhs.r2.z + lhs.r0.w * rhs.r3.z,
                lhs.r0.x * rhs.r0.w + lhs.r0.y * rhs.r1.w + lhs.r0.z * rhs.r2.w + lhs.r0.w * rhs.r3.w
            },

            {
                lhs.r1.x * rhs.r0.x + lhs.r1.y * rhs.r1.x + lhs.r1.z * rhs.r2.x + lhs.r1.w * rhs.r3.x,
                lhs.r1.x * rhs.r0.y + lhs.r1.y * rhs.r1.y + lhs.r1.z * rhs.r2.y + lhs.r1.w * rhs.r3.y,
                lhs.r1.x * rhs.r0.z + lhs.r1.y * rhs.r1.z + lhs.r1.z * rhs.r2.z + lhs.r1.w * rhs.r3.z,
                lhs.r1.x * rhs.r0.w + lhs.r1.y * rhs.r1.w + lhs.r1.z * rhs.r2.w + lhs.r1.w * rhs.r3.w,
            },
            {
                lhs.r2.x * rhs.r0.x + lhs.r2.y * rhs.r1.x + lhs.r2.z * rhs.r2.x + lhs.r2.w * rhs.r3.x,
                lhs.r2.x * rhs.r0.y + lhs.r2.y * rhs.r1.y + lhs.r2.z * rhs.r2.y + lhs.r2.w * rhs.r3.y,
                lhs.r2.x * rhs.r0.z + lhs.r2.y * rhs.r1.z + lhs.r2.z * rhs.r2.z + lhs.r2.w * rhs.r3.z,
                lhs.r2.x * rhs.r0.w + lhs.r2.y * rhs.r1.w + lhs.r2.z * rhs.r2.w + lhs.r2.w * rhs.r3.w,
            },
            {
                lhs.r3.x * rhs.r0.x + lhs.r3.y * rhs.r1.x + lhs.r3.z * rhs.r2.x + lhs.r3.w * rhs.r3.x,
                lhs.r3.x * rhs.r0.y + lhs.r3.y * rhs.r1.y + lhs.r3.z * rhs.r2.y + lhs.r3.w * rhs.r3.y,
                lhs.r3.x * rhs.r0.z + lhs.r3.y * rhs.r1.z + lhs.r3.z * rhs.r2.z + lhs.r3.w * rhs.r3.z,
                lhs.r3.x * rhs.r0.w + lhs.r3.y * rhs.r1.w + lhs.r3.z * rhs.r2.w + lhs.r3.w * rhs.r3.w,
            }
        };
        return result;
    }

    //=============================================================================================================================
    float3 MatrixMultiply(const float3& vec, const float3x3& mat)
    {
        float3 result = {
            vec.x * mat.r0.x + vec.y * mat.r1.x + vec.z * mat.r2.x,
            vec.x * mat.r0.y + vec.y * mat.r1.y + vec.z * mat.r2.y,
            vec.x * mat.r0.z + vec.y * mat.r1.z + vec.z * mat.r2.z
        };
        return result;
    }

    //=============================================================================================================================
    float3 MatrixMultiplyVector(const float3& vec, const float4x4& mat)
    {
        Assert_(mat.r0.w == 0.f);
        Assert_(mat.r1.w == 0.f);
        Assert_(mat.r2.w == 0.f);
        Assert_(mat.r3.w == 1.f);

        float3 result = {
            vec.x * mat.r0.x + vec.y * mat.r1.x + vec.z * mat.r2.x,
            vec.x * mat.r0.y + vec.y * mat.r1.y + vec.z * mat.r2.y,
            vec.x * mat.r0.z + vec.y * mat.r1.z + vec.z * mat.r2.z
        };
        return result;
    }

    //=============================================================================================================================
    float3 MatrixMultiplyPoint(const float3& vec, const float4x4& mat)
    {
        Assert_(mat.r0.w == 0.f);
        Assert_(mat.r1.w == 0.f);
        Assert_(mat.r2.w == 0.f);
        Assert_(mat.r3.w == 1.f);

        float3 result = {
            vec.x * mat.r0.x + vec.y * mat.r1.x + vec.z * mat.r2.x + mat.r3.x,
            vec.x * mat.r0.y + vec.y * mat.r1.y + vec.z * mat.r2.y + mat.r3.y,
            vec.x * mat.r0.z + vec.y * mat.r1.z + vec.z * mat.r2.z + mat.r3.z
        };
        return result;
    }

    //=============================================================================================================================
    float4 MatrixMultiplyFloat4(const float4& vec, const float4x4& mat)
    {
        float4 result = {
            vec.x * mat.r0.x + vec.y * mat.r1.x + vec.z * mat.r2.x + vec.w * mat.r3.x,
            vec.x * mat.r0.y + vec.y * mat.r1.y + vec.z * mat.r2.y + vec.w * mat.r3.y,
            vec.x * mat.r0.z + vec.y * mat.r1.z + vec.z * mat.r2.z + vec.w * mat.r3.z,
            vec.x * mat.r0.w + vec.y * mat.r1.w + vec.z * mat.r2.w + vec.w * mat.r3.w
        };
        return result;
    }

    //=============================================================================================================================
    float4x4 ScreenProjection(uint width, uint height)
    {
        float half_width = 0.5f * width;
        float half_height = 0.5f * height;

        float4x4 result = {
            float4(half_width,        0.0f, 0.0f, 0.0f),
            float4(0.0f, half_height, 0.0f, 0.0f),
            float4(0.0f,        0.0f, 1.0f, 0.0f),
            float4(half_width, half_height, 0.0f, 1.0f)
        };
        return result;
    }

    //=============================================================================================================================
    float4x4 ScreenProjection(float x, float y, uint width, uint height)
    {
        Assert_(width != 0);
        Assert_(height != 0);

        float tow = 2.0f / width;
        float toh = 2.0f / height;

        float4x4 projection_matrix = {
            { tow,              0.f,             0.f, 0.f },
            { 0.f,              -toh,            0.f, 0.f },
            { 0.f,              0.f,             1.f, 0.f },
            { -1.f - (x * tow), 1.f + (y * toh), 0.f, 1.f }
        };

        return projection_matrix;
    }

    //=============================================================================================================================
    float4x4 PerspectiveFovLhProjection(float fov, float aspect, float near, float far)
    {
        float height = 1.0f / Math::Tanf(fov * 0.5f);
        float width = height / aspect;

        float d = far / (far - near);
        float dn = -near * d;

        float4x4 result = {
            float4(width,   0.0f, 0.0f, 0.0f),
            float4(0.0f, height, 0.0f, 0.0f),
            float4(0.0f,   0.0f,    d, 1.0f),
            float4(0.0f,   0.0f,   dn, 0.0f)
        };
        return result;
    }

    //=============================================================================================================================
    float4x4 OffsetCenterProjectionLh(float l, float r, float t, float b, float n, float f)
    {
        float4x4 result = {
            float4(2.0f / (r - l),              0.0f,           0.0f, 0.0f),
            float4(0.0f,    2.0f / (b - t),           0.0f, 0.0f),
            float4(0.0f,              0.0f, 1.0f / (f - n), 0.0f),
            float4((l + r) / (l - r), (t + b) / (t - b),    n / (n - f), 1.0f)
        };
        return result;

    }

    //=============================================================================================================================
    float4x4 LookAtLh(float3 eye, float3 up, float3 target)
    {
        float3 z = Normalize(target - eye);
        float3 x = Normalize(Cross(up, z));
        float3 y = Cross(z, x);

        float3 offset = { -Dot(x, eye), -Dot(y, eye), -Dot(z, eye) };

        float4x4 matrix = {
            float4(x.x, y.x, z.x, 0.f),
            float4(x.y, y.y, z.y, 0.f),
            float4(x.z, y.z, z.z, 0.f),
            float4(offset.x, offset.y, offset.z, 1.f)
        };
        return matrix;
    }

    //=============================================================================================================================
    float4x4 ViewLh(float3 position, float3 forward, float3 up, float3 right)
    {
        float3 z_axis = forward;
        float3 x_axis = right;
        float3 y_axis = up;

        float3 offset = { -Dot(x_axis, position), -Dot(y_axis, position), -Dot(z_axis, position) };

        float4x4 matrix = {
            float4(x_axis.x, y_axis.x, z_axis.x, 0.f),
            float4(x_axis.y, y_axis.y, z_axis.y, 0.f),
            float4(x_axis.z, y_axis.z, z_axis.z, 0.f),
            float4(offset.x, offset.y, offset.z, 1.f)
        };
        return matrix;
    }

    //=============================================================================================================================
    float4x4 PerspectiveFovRhProjection(float fov, float aspect, float near, float far)
    {
        float height = 1.0f / Math::Tanf(fov * 0.5f);
        float width = height / aspect;

        float d = far / (far - near);
        float dn = near * d;

        float4x4 result = {
            float4(width,  0.0f, 0.0f,  0.0f),
            float4(0.0f, height, 0.0f,  0.0f),
            float4(0.0f,   0.0f,    d, -1.0f),
            float4(0.0f,   0.0f,   dn,  0.0f)
        };
        return result;
    }

    //=============================================================================================================================
    float4x4 LookAtRh(float3 eye, float3 up, float3 target)
    {
        float3 z = Normalize(eye - target);
        float3 x = Normalize(Cross(up, z));
        float3 y = Cross(z, x);

        float3 offset = { -Dot(x, eye), -Dot(y, eye), -Dot(z, eye) };

        float4x4 matrix = {
            float4(x.x, y.x, z.x, 0.f),
            float4(x.y, y.y, z.y, 0.f),
            float4(x.z, y.z, z.z, 0.f),
            float4(offset.x, offset.y, offset.z, 1.f)
        };
        return matrix;
    }
}