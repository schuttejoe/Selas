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

    struct SphericalAreaLight
    {
        float3 intensity;
        float3 center;
        float radius;
    };

    struct RectangularAreaLight
    {
        float3 intensity;
        float3 corner;
        float3 eX;
        float3 eZ;
    };

    void InsertRay(KernelContext* context, const Ray& ray);
    void AccumulatePixelEnergy(KernelContext* context, const Ray& ray, float3 value);
    void AccumulatePixelEnergy(KernelContext* context, const HitParameters& hit, float3 value);

    float3 SampleIbl(const ImageBasedLightResourceData* ibl, float3 wi);
    void ShadeSurfaceHit(KernelContext* context, const HitParameters& hit);
}