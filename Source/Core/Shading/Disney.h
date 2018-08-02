#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "MathLib/FloatStructs.h"

namespace Selas
{
    class CSampler;
    struct HitParameters;
    struct SurfaceParameters;
    struct BsdfSample;

    // -- BSDF evaluation for next event estimation
    float3 EvaluateDisneyThin(const SurfaceParameters& surface, float3 v, float3 l, float& forwardPdf, float& reversePdf);

    // -- Shaders
    bool SampleDisneyThin(CSampler* sampler, const SurfaceParameters& surface, float3 v, BsdfSample& sample);
}