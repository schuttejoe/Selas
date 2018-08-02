#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "MathLib/FloatStructs.h"

namespace Selas
{
    namespace Bsdf
    {
        // -- all input vectors assumed to be in tangent space

        float SeparableSmithGGXG1(const float3& w, const float3& wm, float ax, float ay);
        float SeparableSmithGGXG1(const float3& w, float a);

        float GgxAnisotropicD(const float3& wm, float ax, float ay);
        float GgxIsotropicD(const float3& wm, float a);
        
        float3 SampleGgxVndf(const float3& wo, float roughness, float u1, float u2);
        float GgxVndfPdf(const float3& wo, const float3& wm, const float3& wi, float a);

        float3 SampleGgxVndfAnisotropic(const float3& wo, float ax, float ay, float u1, float u2);
        float GgxVndfAnisotropicPdf(const float3& wi, const float3& wm, const float3& wo, float ax, float ay);
        void GgxVndfAnisotropicPdf(const float3& wi, const float3& wm, const float3& wo, float ax, float ay,
                                   float& forwardPdfW, float& reversePdfW);
    }
}