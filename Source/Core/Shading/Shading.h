#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>

struct RTCSceneTy;
typedef struct RTCSceneTy* RTCScene;

namespace Shooty
{
    // -- forward declarations
    struct KernelContext;
    struct HitParameters;
    struct Ray;
    struct Material;
    struct ImageBasedLightResourceData;
    struct SurfaceParameters;
    struct SceneResource;
    namespace Random
    {
        struct MersenneTwister;
    }

    // -- BSDF sample output
    struct BsdfSample
    {
        float3 reflectance = float3::Zero_;
        float3 wi          = float3::Zero_;
        float pdf          = 0.0f;
        bool reflection    = false;
    };

    // JSTODO - KernelContext is bad to pass in here since it gives the shaders access to data they shouldn't have. Need to break it into more nuanced pieces.

    // -- Bsdf evaluation
    void SampleBsdfFunction(KernelContext* context, const SurfaceParameters& surface, BsdfSample& sample);
    float3 EvaluateBsdf(const SurfaceParameters& surface, float3 wo, float3 wi, float& pdf);
}