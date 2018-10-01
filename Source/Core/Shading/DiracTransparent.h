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

    bool SampleDiracTransparent(CSampler* sampler, const SurfaceParameters& surface, float3 v, BsdfSample& sample);
}