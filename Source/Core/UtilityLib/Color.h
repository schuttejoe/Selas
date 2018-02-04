#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>
#include <MathLib/FloatFuncs.h>
#include <SystemLib/CheckedCast.h>
#include <SystemLib/MinMax.h>

namespace Shooty {

    struct ColorRGBA {
        ColorRGBA() {}
        ColorRGBA(uint32 rhs) : rgba(rhs) {}
        ColorRGBA(uint8 r, uint8 g, uint8 b, uint8 a) {
            rgba = ((uint32(a) << 24) | (uint32(b) << 16) | (uint32(g) << 8) | uint32(r));
        }

        inline uint8 r() { return (rgba >> 0) & 0xff; }
        inline uint8 g() { return (rgba >> 8) & 0xff; }
        inline uint8 b() { return (rgba >> 16) & 0xff; }
        inline uint8 a() { return (rgba >> 24) & 0xff; }

        uint32 rgba;
    private:
        ColorRGBA operator=(const uint32& rhs);
        operator uint32() const;
    };

    //==============================================================================
    inline float4 MakeColor4f(ColorRGBA color) {
        uint32 a = color.a();
        uint32 r = color.r();
        uint32 g = color.g();
        uint32 b = color.b();

        static const float inv_channel = 1.f / 255.f;

        float4 color4f = {
            static_cast<float>(r) * inv_channel,
            static_cast<float>(g) * inv_channel,
            static_cast<float>(b) * inv_channel,
            static_cast<float>(a) * inv_channel
        };
        return color4f;
    }

    //==============================================================================
    inline float4 MakeColor4f(uint8 r, uint8 g, uint8 b, uint8 a) {
        return MakeColor4f(ColorRGBA(r, g, b, a));
    }

    //==============================================================================
    inline float3 MakeColor3f(ColorRGBA color) {
        uint32 r = color.r();
        uint32 g = color.g();
        uint32 b = color.b();

        static const float invert = 1.f / 255.f;

        float3 color3f = {
            static_cast<float>(r) * invert,
            static_cast<float>(g) * invert,
            static_cast<float>(b) * invert
        };
        return color3f;
    }

    //==============================================================================
    inline float3 MakeColor3f(uint8 r, uint8 g, uint8 b) {
        return MakeColor3f(ColorRGBA(r, g, b, 255));
    }

}