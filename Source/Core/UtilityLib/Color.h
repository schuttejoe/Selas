#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>
#include <MathLib/FloatFuncs.h>
#include <SystemLib/CheckedCast.h>

namespace Shooty {

    struct ColorRGBA {
        ColorRGBA() {}
        ColorRGBA(uint32 rhs) : rgba(rhs) {}
        ColorRGBA(uint8 r, uint8 g, uint8 b, uint8 a) {
            rgba = ((uint32(a) << 24) | (uint32(b) << 16) | (uint32(g) << 8) | uint32(r));
        }
        ColorRGBA(float4 color) {
            uint8 r = (uint8)(Saturate(color.x) * 255);
            uint8 g = (uint8)(Saturate(color.y) * 255);
            uint8 b = (uint8)(Saturate(color.z) * 255);
            uint8 a = (uint8)(Saturate(color.w) * 255);

            rgba = ((uint32(a) << 24) | (uint32(b) << 16) | (uint32(g) << 8) | uint32(r));
        }

        inline uint8 r() { return (rgba >> 0) & 0xff; }
        inline uint8 g() { return (rgba >> 8) & 0xff; }
        inline uint8 b() { return (rgba >> 16) & 0xff; }
        inline uint8 a() { return (rgba >> 24) & 0xff; }

        uint32 rgba;

        operator uint32() const { return rgba; }
    };

    //==============================================================================
    inline float4 MakeColor4f(ColorRGBA color) {
        uint32 a = color.a();
        uint32 r = color.r();
        uint32 g = color.g();
        uint32 b = color.b();

        static const float invertColor = 1.f / 255.f;

        float4 color4f = {
            static_cast<float>(r) * invertColor,
            static_cast<float>(g) * invertColor,
            static_cast<float>(b) * invertColor,
            static_cast<float>(a) * invertColor
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

        static const float invertColor = 1.0f / 255.0f;

        float3 color3f = {
            static_cast<float>(r) * invertColor,
            static_cast<float>(g) * invertColor,
            static_cast<float>(b) * invertColor
        };
        return color3f;
    }

    //==============================================================================
    inline float3 MakeColor3f(uint8 r, uint8 g, uint8 b) {
        return MakeColor3f(ColorRGBA(r, g, b, 255));
    }

}