#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "MathLib/FloatStructs.h"

struct RTCSceneTy;
typedef struct RTCSceneTy* RTCScene;

namespace Selas
{
    // -- forward declarations
    class CSampler;
    struct HitParameters;
    struct Ray;
    struct Material;
    struct ImageBasedLightResourceData;
    struct SurfaceParameters;
    struct SceneResource;

    // -- BSDF sample output
    struct BsdfSample
    {
        float3 reflectance = float3::Zero_;
        float3 wi          = float3::Zero_;
        float forwardPdfW  = 0.0f;
        float reversePdfW  = 0.0f;
    };

    // -- Bsdf evaluation
    bool SampleBsdfFunction(CSampler* sampler, const SurfaceParameters& surface, float3 v, BsdfSample& sample);
    float3 EvaluateBsdf(const SurfaceParameters& surface, float3 wo, float3 wi, float& forwardPdf, float& reversePdf);
}