#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "MathLib/FloatStructs.h"

namespace Selas
{
    namespace Fresnel
    {
        float3 Schlick(float3 r0, float radians);
        float Schlick(float r0, float radians);
        float SchlickWeight(float u);
        float SchlickDielectic(float cosThetaI, float relativeIor);
        float Dielectric(float cosThetaI, float relativeIorI, float relativeIorT);
        float SchlickR0FromRelativeIOR(float eta);
    }
}