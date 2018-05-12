#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>

namespace Shooty
{
    struct KernelContext;
    struct HitParameters;
    struct SurfaceParameters;
    
    // -- BSDF evaluation for next event estimation
    float3 CalculateDisneyBsdf(const SurfaceParameters& surface, float3 wo, float3 wi);

    // -- Shaders
    void DisneyWithIblSamplingShader(KernelContext* __restrict context, const HitParameters& hit, const SurfaceParameters& surface);
    void DisneyBrdfShader(KernelContext* __restrict context, const HitParameters& hit, const SurfaceParameters& surface);
}