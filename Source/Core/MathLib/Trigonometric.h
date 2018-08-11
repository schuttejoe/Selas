#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "MathLib/FloatStructs.h"
#include "SystemLib/MinMax.h"

namespace Selas
{
    namespace Math
    {
        const float Pi_               = 3.141592653589793f;
        const float TwoPi_            = Pi_ * 2.0f;
        const float PiSquared_        = Pi_ * Pi_;
        const float InvPi_            = 1.0f / Pi_;
        const float Inv4Pi_           = 1.0f / (4.0f * Pi_);

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
        float Ln(float x);
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

        //=========================================================================================================================
        // These all assume vectors are in a y-up tangent space. Credit to PBRT for the idea to design these this way.
        //=========================================================================================================================
        ForceInline_ float CosTheta(const float3& w)
        {
            return w.y;
        }

        ForceInline_ float Cos2Theta(const float3& w)
        {
            return w.y * w.y;
        }

        ForceInline_ float AbsCosTheta(const float3& w)
        {
            return Absf(CosTheta(w));
        }

        ForceInline_ float Sin2Theta(const float3& w)
        {
            return Max(0.0f, 1.0f - Cos2Theta(w));
        }

        ForceInline_ float SinTheta(const float3& w)
        {
            return Sqrtf(Sin2Theta(w));
        }

        ForceInline_ float TanTheta(const float3& w)
        {
            return SinTheta(w) / CosTheta(w);
        }

        ForceInline_ float Tan2Theta(const float3& w)
        {
            return Sin2Theta(w) / Cos2Theta(w);
        }

        ForceInline_ float CosPhi(const float3& w)
        {
            float sinTheta = SinTheta(w);
            return (sinTheta == 0) ? 1.0f : Clamp(w.x / sinTheta, -1.0f, 1.0f);
        }

        ForceInline_ float SinPhi(const float3& w)
        {
            float sinTheta = SinTheta(w);
            return (sinTheta == 0) ? 1.0f : Clamp(w.z / sinTheta, -1.0f, 1.0f);
        }

        ForceInline_ float Cos2Phi(const float3& w)
        {
            float cosPhi = CosPhi(w);
            return cosPhi * cosPhi;
        }

        ForceInline_ float Sin2Phi(const float3& w)
        {
            float sinPhi = SinPhi(w);
            return sinPhi * sinPhi;
        }
        
    }
}