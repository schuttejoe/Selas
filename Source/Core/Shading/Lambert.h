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

    float3 EvaluateLambert(const SurfaceParameters& surface, const float3& v, const float3& l,
                           float& forwardPdf, float& reversePdf);

    bool SampleLambert(CSampler* sampler, const SurfaceParameters& surface, const float3& v, BsdfSample& sample);
}