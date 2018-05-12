
//==============================================================================
// Joe Schutte
//==============================================================================

#include <Bsdf/Ggx.h>

#include <MathLib/Trigonometric.h>
#include <MathLib/FloatFuncs.h>
#include <SystemLib/MinMax.h>

namespace Shooty
{
    namespace Bsdf
    {
        //==============================================================================
        static float BsdfNDot(float3 x)
        {
            return x.y;
        }

        //==============================================================================
        float SmithGGXMasking(float3 wi, float3 wo, float3 wm, float a2)
        {
            float dotNL = Math::Absf(BsdfNDot(wi));
            float dotNV = Math::Absf(BsdfNDot(wo));
            float denomC = Math::Sqrtf(a2 + (1.0f - a2) * dotNV * dotNV) + dotNV;

            return 2.0f * dotNV / denomC;
        }

        //==============================================================================
        float SmithGGXMaskingShading(float3 wi, float3 wo, float3 wm, float a2)
        {
            // non height-correlated masking function
            // https://twvideo01.ubm-us.net/o1/vault/gdc2017/Presentations/Hammon_Earl_PBR_Diffuse_Lighting.pdf
            float dotNL = Math::Absf(BsdfNDot(wi));
            float dotNV = Math::Absf(BsdfNDot(wo));

            float denomA = dotNV * Math::Sqrtf(a2 + (1.0f - a2) * dotNL * dotNL);
            float denomB = dotNL * Math::Sqrtf(a2 + (1.0f - a2) * dotNV * dotNV);

            return 2.0f * dotNL * dotNV / (denomA + denomB);
        }

        //==============================================================================
        void GgxD(float roughness, float r0, float r1, float& theta, float& phi)
        {
            float m2 = roughness * roughness;

            phi = Math::TwoPi_ * r0;
            theta = Math::Acosf(Math::Sqrtf((1.0f - r1) / ((m2 - 1.0f) * r1 + 1.0f)));
        }

        //==============================================================================
        float GgxDPdf(float roughness, float dotNH)
        {
            float a2 = roughness * roughness;
            float sqrtdenom = (dotNH * dotNH) * (a2 - 1) + 1;

            return a2 / (Math::Pi_ * sqrtdenom * sqrtdenom);
        }

        //==============================================================================
        // https://hal.archives-ouvertes.fr/hal-01509746/document
        float3 GgxVndf(float3 wo, float roughness, float u1, float u2)
        {
            // -- Stretch the view vector so we are sampling as though roughness==1
            float3 v = Normalize(float3(wo.x * roughness, wo.y, wo.z * roughness));

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
            return Normalize(float3(roughness * n.x, Max<float>(0.0f, n.y), roughness * n.z));
        }

        //==============================================================================
        float GgxVndfPdf(float roughness, float3 wo, float3 wm, float3 wi)
        {
            float a2 = roughness * roughness;

            float dotLH = Math::Absf(Dot(wi, wm));
            float dotLN = Math::Absf(BsdfNDot(wi));
            float dotNH = Math::Absf(BsdfNDot(wm));
            float G1 = Bsdf::SmithGGXMasking(wi, wo, wm, a2);
            float D = Bsdf::GgxDPdf(roughness, dotNH);

            return G1 * dotLH * D / dotLN;
        }
    }
}