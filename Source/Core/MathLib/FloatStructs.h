#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SystemLib/BasicTypes.h"

namespace Selas
{
    struct float2
    {
        float x, y;

        ForceInline_ float2() {}
        ForceInline_ float2(float v) : x(v), y(v) {}
        ForceInline_ float2(float x_, float y_) : x(x_), y(y_) {}

        static const float2 Zero_;
        static const float2 XAxis_;
        static const float2 YAxis_;
    };

    struct float3
    {
        float x, y, z;

        ForceInline_ float3() {}
        ForceInline_ float3(float v) : x(v), y(v), z(v) {}
        ForceInline_ float3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

        static const float3 Zero_;
        static const float3 One_;
        static const float3 XAxis_;
        static const float3 YAxis_;
        static const float3 ZAxis_;
    };

    struct float4
    {
        float x, y, z, w;
        ForceInline_ float4() {}
        ForceInline_ float4(float v) : x(v), y(v), z(v), w(v) {}
        ForceInline_ float4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
        ForceInline_ float4(float3 xyz, float w_) : x(xyz.x), y(xyz.y), z(xyz.z), w(w_) {}

        ForceInline_ float3 XYZ() { return float3(x, y, z); }

        static const float4 Zero_;
        static const float4 XAxis_;
        static const float4 YAxis_;
        static const float4 ZAxis_;
    };

    struct float2x2
    {
        float2 r0;
        float2 r1;
    };

    ForceInline_ float2x2 MakeFloat2x2(float2 r0, float2 r1)
    {
        float2x2 result = { r0, r1 };
        return result;
    }

    struct float3x3
    {
        float3 r0;
        float3 r1;
        float3 r2;
    };

    ForceInline_ float3x3 MakeFloat3x3(float3 r0, float3 r1, float3 r2)
    {
        float3x3 result = { r0, r1, r2 };
        return result;
    }

    struct float3x4
    {
        float4 r1;
        float4 r2;
        float4 r3;
    };

    ForceInline_ float3x4 MakeFloat3x4(float4 r1, float4 r2, float4 r3)
    {
        float3x4 result = { r1, r2, r3 };
        return result;
    }

    struct float4x4
    {
        float4 r0;
        float4 r1;
        float4 r2;
        float4 r3;
    };

    ForceInline_ float4x4 MakeFloat4x4(float4 r1, float4 r2, float4 r3, float4 r4)
    {
        float4x4 result = { r1, r2, r3, r4 };
        return result;
    }
}
