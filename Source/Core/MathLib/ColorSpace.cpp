//==============================================================================
// Joe Schutte
//==============================================================================

#include "MathLib/ColorSpace.h"
#include "MathLib/FloatFuncs.h"
#include "MathLib/Trigonometric.h"

namespace Selas
{
    namespace Math
    {
        // Precise functions come directly from https://en.wikipedia.org/wiki/SRGB

        //==============================================================================
        float SrgbToLinearPrecise(float x)
        {
            if(x <= 0.04045f) {
                return x / 12.92f;
            }

            return Math::Powf((x + 0.055f) / (1.0f + 0.055f), 2.4f);
        }

        //==============================================================================
        float2 SrgbToLinearPrecise(float2 srgb)
        {
            float2 result;
            result.x = SrgbToLinearPrecise(srgb.x);
            result.y = SrgbToLinearPrecise(srgb.y);

            return result;
        }

        //==============================================================================
        float3 SrgbToLinearPrecise(float3 srgb)
        {
            float3 result;
            result.x = SrgbToLinearPrecise(srgb.x);
            result.y = SrgbToLinearPrecise(srgb.y);
            result.z = SrgbToLinearPrecise(srgb.z);

            return result;
        }

        //==============================================================================
        float4 SrgbToLinearPrecise(float4 srgb)
        {
            float4 result;
            result.x = SrgbToLinearPrecise(srgb.x);
            result.y = SrgbToLinearPrecise(srgb.y);
            result.z = SrgbToLinearPrecise(srgb.z);
            result.w = SrgbToLinearPrecise(srgb.w);

            return result;
        }

        //==============================================================================
        float LinearToSrgbPrecise(float x)
        {
            if(x < 0.0031308f) {
                return 12.92f * x;
            }

            return (1.0f + 0.055f) * Math::Powf(x, 1.0f / 2.4f) - 0.055f;
        }

        //==============================================================================
        float3 LinearToSrgbPrecise(float3 linear)
        {
            float3 srgb;
            srgb.x = LinearToSrgbPrecise(linear.x);
            srgb.y = LinearToSrgbPrecise(linear.y);
            srgb.z = LinearToSrgbPrecise(linear.z);

            return srgb;
        }
    }
}