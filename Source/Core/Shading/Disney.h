#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "MathLib/FloatStructs.h"

namespace Selas
{
    struct KernelContext;
    struct HitParameters;
    struct SurfaceParameters;
    struct BsdfSample;

    // -- BSDF evaluation for next event estimation
    float3 EvaluateDisneyBrdf(const SurfaceParameters& surface, float3 wo, float3 wi, float& forwardPdf, float& reversePdf);

    // -- Shaders
    bool SampleIblWithDisneyBrdf(KernelContext* context, const SurfaceParameters& surface, float3 v, BsdfSample& sample);
    bool SampleDisneyBrdf(KernelContext* context, const SurfaceParameters& surface, float3 v, BsdfSample& sample);
}