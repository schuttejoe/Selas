#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>

namespace Shooty
{
    namespace Bsdf
    {
        float SmithGGXMasking(float absDotNV, float a2);
        float SmithGGXMaskingShading(float absDotNL, float absDotNV, float a2);

        void GgxD(float roughness, float r0, float r1, float& theta, float& phi);
        float GgxDPdf(float dotNH, float a2);
        
        float3 GgxVndf(float3 wo, float roughness, float u1, float u2);
        float GgxVndfPdf(float absDotLH, float absDotNL, float absDotNV, float absDotNH, float a2);
    }
}