#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>

struct RTCSceneTy;
typedef struct RTCSceneTy* RTCScene;

namespace Shooty
{
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

    Ray CreateReflectionBounceRay(const SurfaceParameters& surface, const HitParameters& hit, float3 wi, float3 reflectance);
    Ray CreateRefractionBounceRay(const SurfaceParameters& surface, const HitParameters& hit, float3 wi, float3 reflectance, float iorRatio);

    void InsertRay(KernelContext* context, const Ray& ray);
    void AccumulatePixelEnergy(KernelContext* context, const Ray& ray, float3 value);
    void AccumulatePixelEnergy(KernelContext* context, const HitParameters& hit, float3 value);

    float3 SampleIbl(const ImageBasedLightResourceData* ibl, float3 wi);
    void ShadeSurfaceHit(KernelContext* context, const HitParameters& hit);

    float3 EvaluateBsdf(const SurfaceParameters& surface, float3 wo, float3 wi, float& pdf);
}