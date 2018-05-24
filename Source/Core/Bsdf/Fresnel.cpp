
//==============================================================================
// Joe Schutte
//==============================================================================

#include <Bsdf/Fresnel.h>

#include <MathLib/Trigonometric.h>
#include <MathLib/FloatFuncs.h>

namespace Selas
{
    namespace Fresnel
    {
        //==============================================================================
        float3 Schlick(float3 r0, float radians)
        {
            float exponential = Math::Powf(1.0f - radians, 5.0f);
            return r0 + float3(1.0f - r0.x, 1.0f - r0.y, 1.0f - r0.z) * exponential;
        }

        //==============================================================================
        float Schlick(float u)
        {
            float m = Saturate(1.0f - u);
            float m2 = m * m;
            return m * m2 * m2;
        }

        //==============================================================================
        float SchlickDialectic(float cosThetaI, float ni, float nt)
        {
            float r0 = (ni - 1.0f) / (nt + 1.0f);
            r0 *= r0;

            return r0 + (1.0f - r0) * Math::Powf(1 - cosThetaI, 5.0f);
        }
    }
}