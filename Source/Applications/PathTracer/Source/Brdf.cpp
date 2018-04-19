
//==============================================================================
// Joe Schutte
//==============================================================================

#include "Brdf.h"

#include <MathLib/FloatStructs.h>
#include <MathLib/FloatFuncs.h>
#include <MathLib/Trigonometric.h>

namespace Shooty
{
    // Sources:
    // Physically Based Shading at Disney:
    // http://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf
    // Crafting a Next-Gen material pipeline for The Order: 1886:
    // http://blog.selfshadow.com/publications/s2013-shading-course/rad/s2013_pbs_rad_notes.pdf

    // ================================================================================================
    // Fresnel
    // ================================================================================================
    static float3 SchlickFresnel(float3 reflectance, float angle)
    {
        float exponential = Math::Powf(1.0f - angle, 5.0f);
        return reflectance + float3(1.0f - reflectance.x, 1.0f - reflectance.y, 1.0f - reflectance.z) * exponential;
    }

    // ================================================================================================
    // The visibility term used for microfacet BRDF's.
    // Taken from "Distribution-based BRDFs" [Ashikhmin 07]
    // ================================================================================================
    static float AshikhminMicrofacetVisibilityTerm(float ndotl, float ndotv)
    {
        return 1.0f / (ndotl + ndotv - ndotl * ndotv);
    }

    // ===============================================================================================
    // Using an optimized version of [Neubelt + Pettineo 2013]. https://github.com/TheRealMJP
    // ===============================================================================================
    float GGX_Specular(float m, float3 n, float3 h, float3 v, float3 l)
    {
        float ndoth = Math::Absf(Dot(n, h));
        float ndoth2 = ndoth * ndoth;
        float ndoth4 = ndoth2 * ndoth2;
        float ndotl = Saturate(Dot(n, l));
        float ndotl2 = ndotl * ndotl;
        float ndotv = Saturate(Dot(n, v));
        float ndotv2 = ndotv * ndotv;

        float adjusted_m = 0.5f + m * 0.5f;
        float adjusted_m2 = adjusted_m * adjusted_m;
        float m2 = m * m;

        // -- calculate the distribution term
        float tantheta2 = (1 - ndoth2) / ndoth2;
        float d = m2 / (Math::Pi_ * ndoth4 * (m2 + tantheta2) * (m2 + tantheta2));

        // -- calculate the matching geometric term
        tantheta2 = (1 - ndotl2) / ndotl2;
        float g1i = 2.0f / (1 + Math::Sqrtf(1 + adjusted_m2 * tantheta2));
        tantheta2 = (1 - ndotv2) / (ndotv2 + 0.00001f);
        float g1o = 2.0f / (1 + Math::Sqrtf(1 + adjusted_m2 * tantheta2));
        float g = g1i * g1o;

        // -- visibility term
        float vis = AshikhminMicrofacetVisibilityTerm(ndotl, ndotv);

        return d;// *g * vis * (1.0f / 4.0f);
    }

    //==============================================================================
    float3 GgxBrdf(float3 n, float3 l, float3 v, float3 albedo, float3 reflectance, float roughness)
    {
        float ndotl = Dot(n, l);
        float3 direct = float3::Zero_;

        if(ndotl > 0.0f) {

            ndotl = Saturate(ndotl);

            // -- Diffuse
            direct += albedo * Math::OOPi_;

            // -- Specular
            float3 h = Normalize(l + v);

            float vdoth = Saturate(Dot(v, h));
            float sspec = GGX_Specular(roughness, n, h, v, l);
            float3 specular = sspec * SchlickFresnel(reflectance, vdoth);

            direct += specular;
        }

        return direct * ndotl;
    }
}