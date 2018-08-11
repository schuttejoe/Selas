#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "Shading/Scattering.h"
#include "MathLib/FloatStructs.h"

namespace Selas
{
    class CSampler;

    float SampleDistance(CSampler* sampler, const MediumParameters& medium, float* pdf);
    float3 SampleScatterDirection(CSampler* sampler, const MediumParameters& medium, float3 wo, float* pdf);
    float ScatterDirectionPdf(const MediumParameters& medium, float3 wo, float3 wi);
    float3 Transmission(const MediumParameters& medium, float distance);
}