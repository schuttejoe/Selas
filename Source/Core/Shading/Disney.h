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
    float3 EvaluateDisney(const SurfaceParameters& surface, float3 v, float3 l, bool thin, float& forwardPdf, float& reversePdf);

    // -- Shaders
    bool SampleDisney(CSampler* sampler, const SurfaceParameters& surface, float3 v, bool thin, BsdfSample& sample);
}