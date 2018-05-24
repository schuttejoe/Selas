
//==============================================================================
// Joe Schutte
//==============================================================================

#include <Bsdf/Ggx.h>

#include <MathLib/Trigonometric.h>
#include <MathLib/FloatFuncs.h>
#include <SystemLib/MinMax.h>

namespace Selas
{
    namespace Bsdf
    {
        //==============================================================================
        float SmithGGXMasking(float absDotNV, float a2)
        {
            float denomC = Math::Sqrtf(a2 + (1.0f - a2) * absDotNV * absDotNV) + absDotNV;

            return 2.0f * absDotNV / denomC;
        }

        //==============================================================================
        float SmithGGXMaskingShading(float absDotNL, float absDotNV, float a2)
        {
            // non height-correlated masking function
            // https://twvideo01.ubm-us.net/o1/vault/gdc2017/Presentations/Hammon_Earl_PBR_Diffuse_Lighting.pdf
            float denomA = absDotNV * Math::Sqrtf(a2 + (1.0f - a2) * absDotNL * absDotNL);
            float denomB = absDotNL * Math::Sqrtf(a2 + (1.0f - a2) * absDotNV * absDotNV);

            return 2.0f * absDotNL * absDotNV / (denomA + denomB);
        }

        //==============================================================================
        void GgxD(float roughness, float r0, float r1, float& theta, float& phi)
        {
            float m2 = roughness * roughness;

            phi = Math::TwoPi_ * r0;
            theta = Math::Acosf(Math::Sqrtf((1.0f - r1) / ((m2 - 1.0f) * r1 + 1.0f)));
        }

        //==============================================================================
        float GgxDPdf(float dotNH, float a2)
        {
            float sqrtdenom = (dotNH * dotNH) * (a2 - 1) + 1;

            return a2 / (Math::Pi_ * sqrtdenom * sqrtdenom);
        }

        //==============================================================================
        // https://hal.archives-ouvertes.fr/hal-01509746/document
        float3 GgxVndf(float3 wo, float roughness, float u1, float u2)
        {
            float sign = Math::Sign(wo.y);

            // -- Stretch the view vector so we are sampling as though roughness==1
            float3 v = sign * Normalize(float3(wo.x * roughness, wo.y, wo.z * roughness));

            // -- Build an orthonormal basis with v, t1, and t2
            float3 t1 = (v.y < 0.9999f) ? Normalize(Cross(v, float3::YAxis_)) : float3::XAxis_;
            float3 t2 = Cross(t1, v);

            // -- Choose a point on a disk with each half of the disk weighted proportionally to its projection onto direction v
            float a = 1.0f / (1.0f + v.y);
            float r = Math::Sqrtf(u1);
            float phi = (u2 < a) ? (u2 / a) * Math::Pi_ : Math::Pi_ + (u2 - a) / (1.0f - a) * Math::Pi_;
            float p1 = r * Math::Cosf(phi);
            float p2 = r * Math::Sinf(phi) * ((u2 < a) ? 1.0f : v.y);

            // -- Calculate the normal in this stretched tangent space
            float3 n = p1 * t1 + p2 * t2 + Math::Sqrtf(Max<float>(0.0f, 1.0f - p1 * p1 - p2 * p2)) * v;

            // -- unstretch and normalize the normal
            return sign * Normalize(float3(roughness * n.x, Max<float>(0.0f, n.y), roughness * n.z));
        }

        //==============================================================================
        float GgxVndfPdf(float absDotLH, float absDotNL, float absDotNV, float absDotNH, float a2)
        {
            float G1 = Bsdf::SmithGGXMasking(absDotNV, a2);
            float D = Bsdf::GgxDPdf(absDotNH, a2);

            return G1 * absDotLH * D / absDotNL;
        }
    }
}