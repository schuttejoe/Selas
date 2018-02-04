//==============================================================================
// Joe Schutte
//==============================================================================

#include "Trigonometric.h"
#include <math.h>

namespace Shooty {

    //==============================================================================
    float Math::Cosf(float radians)
    {
        return ::cosf(radians);
    }

    //==============================================================================
    float Math::Sinf(float radians)
    {
        return ::sinf(radians);
    }

    //==============================================================================
    float Math::Tanf(float radians)
    {
        return ::tanf(radians);
    }

    //==============================================================================
    float Math::Cotf(float radians)
    {
        return Cosf(radians) / Sinf(radians);
    }
    
    //==============================================================================
    float Math::Acosf(float radians)
    {
        return ::acosf(radians);
    }

    //==============================================================================
    float Math::Atanf(float radians)
    {
        return ::atanf(radians);
    }

    //==============================================================================
    float Math::Atan2f(float x, float y)
    {
        return ::atan2f(x, y);
    }

    //==============================================================================
    float Math::Fmodf(float x, float y)
    {
        return ::fmodf(x, y);
    }

    //==============================================================================
    float Math::Expf(float x)
    {
        return static_cast<float>(::expf(x));
    }

    //==============================================================================
    float Math::Sqrtf(float x)
    {
        return static_cast<float>(::sqrtf(x));
    }

    //==============================================================================
    float Math::Absf(float x)
    {
        return (x < 0) ? -x : x;
    }

    //==============================================================================
    float Math::Powf(float x, float y) {
        return ::powf(x, y);
    }

}