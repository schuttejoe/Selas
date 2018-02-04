#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Shooty {

    struct float2
    {
        float x, y;

        ForceInline_ float2() {}
        ForceInline_ float2(float x_, float y_) : x(x_), y(y_) { }
    };

    struct float3
    {
        float x, y, z;

        ForceInline_ float3() {}
        ForceInline_ float3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) { }
    };

    struct float4
    {
        float x, y, z, w;
        ForceInline_ float4() {}
        ForceInline_ float4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) { }
        ForceInline_ float4(float3 xyz, float w_) : x(xyz.x), y(xyz.y), z(xyz.z), w(w_) { }
    };

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

