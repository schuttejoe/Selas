#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SceneLib/SceneResource.h>
#include <SceneLib/ImageBasedLightResource.h>
#include <GeometryLib/Camera.h>
#include <GeometryLib/Ray.h>
#include <MathLib/Random.h>
#include <SystemLib/BasicTypes.h>

struct RTCSceneTy;
typedef struct RTCSceneTy* RTCScene;

namespace Shooty
{
    struct SceneResource;
    struct ImageBasedLightResource;
    struct RayCastCameraSettings;
    struct SurfaceParameters;

    namespace Random
    {
        struct MersenneTwister;
    }

    //==============================================================================
    struct SceneContext
    {
        RTCScene rtcScene;
        const SceneResource* scene;
        const ImageBasedLightResourceData* ibl;
    };

    //==============================================================================
    struct KernelContext
    {
        const RayCastCameraSettings* __restrict camera;
        SceneContext*                __restrict sceneData;
        Random::MersenneTwister*     __restrict twister;
        float3*                      __restrict imageData;
        uint                                    imageWidth;
        uint                                    imageHeight;
        uint                                    maxPathLength;

        // JSTODO - This probably won't match the behavior I want for deferred path tracing. Delete me.
        Ray* __restrict rayStack;
        uint rayStackCount;
        uint rayStackCapacity;
    };

    //==============================================================================
    struct HitParameters
    {
        float3 position;
        float error;
        float3 viewDirection;
        int32 primId;

        float2 baryCoords;

        float3 rxOrigin;
        float3 rxDirection;
        float3 ryOrigin;
        float3 ryDirection;

        uint32 bounceCount;
        uint32 pixelIndex;
        float3 throughput;
    };

    struct PathState
    {
        float3 position;
        float3 direction;
        float3 throughput;
        float dVCM;
        float dVC;
        float dVM;
        uint32 pathLength : 31;
        uint32 isAreaMeasure : 1;
    };

    // -- generation of differential rays
    Ray CreateReflectionBounceRay(const SurfaceParameters& surface, const HitParameters& hit, float3 wi, float3 reflectance);
    Ray CreateRefractionBounceRay(const SurfaceParameters& surface, const HitParameters& hit, float3 wi, float3 reflectance, float iorRatio);

    // -- context utility functions
    void AccumulatePixelEnergy(KernelContext* context, const Ray& ray, float3 value);
    void AccumulatePixelEnergy(KernelContext* context, const HitParameters& hit, float3 value);
    void InsertRay(KernelContext* context, const Ray& ray);
}