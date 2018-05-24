#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

// JSTODO - Rename this file or split it into more than one.

namespace Selas
{
    namespace Math
    {
        const float Pi_ = 3.141592653589793f;
        const float TwoPi_ = Pi_ * 2.0f;
        const float PiSquared_ = Pi_ * Pi_;
        const float OOPi_ = 1.0f / Pi_;
        const float DegreesToRadians_ = Pi_ / 180.f;

        float Cosf(float radians);
        float Sinf(float radians);
        float Tanf(float radians);
        float Cotf(float radians);
        float Acosf(float radians);
        float Asinf(float radians);
        float Atanf(float radians);
        float Atan2f(float x, float y);

        // JSTODO -- These don't belong here
        float Fmodf(float x, float y);
        float Expf(float x);
        float Log2(float x);
        float Sqrtf(float x);
        float Absf(float x);
        float Powf(float x, float y);
        float Square(float x);
        float Floor(float x);
        float Ceil(float x);
        float Sign(float x);

        bool IsInf(float x);
        bool IsNaN(float x);
    }
}