#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

namespace Shooty {

    namespace Math {

        const float Pi_ = 3.141592653589793f;
        const float DegreesToRadians_ = Pi_ / 180.f;

        float Cosf(float radians);
        float Sinf(float radians);
        float Tanf(float radians);
        float Cotf(float radians);
        float Acosf(float radians);
        float Atanf(float radians);
        float Atan2f(float x, float y);
        float Fmodf(float x, float y);
        float Expf(float x);
        float Sqrtf(float x);
        float Absf(float x);
        float Powf(float x, float y);

    }

}