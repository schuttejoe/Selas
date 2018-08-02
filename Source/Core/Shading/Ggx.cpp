
//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "Shading/Ggx.h"

#include "MathLib/Trigonometric.h"
#include "MathLib/FloatFuncs.h"
#include "SystemLib/MinMax.h"

namespace Selas
{
    using namespace Math;

    namespace Bsdf
    {
        //=============================================================================================================================
        float SeparableSmithGGXG1(const float3& w, const float3& wm, float ax, float ay)
        {
            float dotHW = Dot(w, wm);
            if (dotHW <= 0.0f) {
                return 0.0f;
            }

            float absTanTheta = Absf(TanTheta(w));
            if(IsInf(absTanTheta)) {
                return 0.0f;
            }

            float a = Sqrtf(Cos2Phi(w) * ax * ax + Sin2Phi(w) * ay * ay);
            float a2Tan2Theta = Square(a * absTanTheta);

            float lambda = 0.5f * (-1.0f + Sqrtf(1.0f + a2Tan2Theta));
            return 1.0f / (1.0f + lambda);
        }

        //=========================================================================================================================
        float SeparableSmithGGXG1(const float3& w, float a)
        {
            float a2 = a * a;
            float absDotNV = AbsCosTheta(w);

            return 2.0f / (1.0f + Math::Sqrtf(a2 + (1 - a2) * absDotNV * absDotNV));
        }

        //=========================================================================================================================
        float HeightCorrelatedSmithGGXG2(const float3& wo, const float3& wi, float a)
        {
            float absDotNV = AbsCosTheta(wo);
            float absDotNL = AbsCosTheta(wi);
            float a2 = a * a;

            // height-correlated masking function
            // https://twvideo01.ubm-us.net/o1/vault/gdc2017/Presentations/Hammon_Earl_PBR_Diffuse_Lighting.pdf
            float denomA = absDotNV * Math::Sqrtf(a2 + (1.0f - a2) * absDotNL * absDotNL);
            float denomB = absDotNL * Math::Sqrtf(a2 + (1.0f - a2) * absDotNV * absDotNV);

            return 2.0f * absDotNL * absDotNV / (denomA + denomB);
        }

        //=========================================================================================================================
        float GgxIsotropicD(const float3& wm, float a)
        {
            float a2 = a * a;
            float dotNH2 = Cos2Theta(wm);

            float sqrtdenom = dotNH2 * (a2 - 1) + 1;
            return a2 / (Math::Pi_ * sqrtdenom * sqrtdenom);
        }

        //=========================================================================================================================
        float GgxAnisotropicD(const float3& wm, float ax, float ay)
        {
            float dotHX2 = Square(wm.x);
            float dotHY2 = Square(wm.z);
            float cos2Theta = Cos2Theta(wm);
            float ax2 = Square(ax);
            float ay2 = Square(ay);

            return 1.0f / (Math::Pi_ * ax * ay * Square(dotHX2 / ax2 + dotHY2 / ay2 + cos2Theta));
        }

        //=========================================================================================================================
        // https://hal.archives-ouvertes.fr/hal-01509746/document
        float3 SampleGgxVndf(float3 wo, float roughness, float u1, float u2)
        {
            return SampleGgxVndfAnisotropic(wo, roughness, roughness, u1, u2);
        }

        //=========================================================================================================================
        float GgxVndfPdf(const float3& wo, const float3& wm, const float3& wi, float a)
        {
            float absDotNL = AbsCosTheta(wi);
            float absDotLH = Absf(Dot(wm, wi));

            float G1 = Bsdf::SeparableSmithGGXG1(wo, a);
            float D = Bsdf::GgxIsotropicD(wm, a);

            return G1 * absDotLH * D / absDotNL;
        }

        //=========================================================================================================================
        float3 SampleGgxVndfAnisotropic(const float3& wo, float ax, float ay, float u1, float u2)
        {
            float sign = Math::Sign(wo.y);

            // -- Stretch the view vector so we are sampling as though roughness==1
            float3 v = sign * Normalize(float3(wo.x * ax, wo.y, wo.z * ay));

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
            return sign * Normalize(float3(ax * n.x, Max<float>(0.0f, n.y), ay * n.z));
        }

        //=========================================================================================================================
        float GgxVndfAnisotropicPdf(const float3& wi, const float3& wm, const float3& wo, float ax, float ay)
        {
            float absDotNL = AbsCosTheta(wi);
            float absDotLH = Absf(Dot(wm, wi));

            float G1 = Bsdf::SeparableSmithGGXG1(wo, wm, ax, ay);
            float D = Bsdf::GgxAnisotropicD(wm, ax, ay);

            return G1 * absDotLH * D / absDotNL;
        }

        //=========================================================================================================================
        void GgxVndfAnisotropicPdf(const float3& wi, const float3& wm, const float3& wo, float ax, float ay,
                                   float& forwardPdfW, float& reversePdfW)
        {
            float D = Bsdf::GgxAnisotropicD(wm, ax, ay);

            float absDotNL = AbsCosTheta(wi);
            float absDotHL = Absf(Dot(wm, wi));
            float G1v = Bsdf::SeparableSmithGGXG1(wo, wm, ax, ay);
            forwardPdfW = G1v * absDotHL * D / absDotNL;

            float absDotNV = AbsCosTheta(wo);
            float absDotHV = Absf(Dot(wm, wo));
            float G1l = Bsdf::SeparableSmithGGXG1(wi, wm, ax, ay);
            reversePdfW = G1l * absDotHV * D / absDotNV;
        }
    }
}