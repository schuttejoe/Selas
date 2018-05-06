#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SceneLib/SceneResource.h>
#include <SceneLib/ImageBasedLightResource.h>
#include <GeometryLib/Camera.h>
#include <MathLib/Random.h>
#include <SystemLib/BasicTypes.h>

struct RTCSceneTy;
typedef struct RTCSceneTy* RTCScene;

namespace Shooty
{
    struct SceneResource;
    struct ImageBasedLightResource;
    struct RayCastCameraSettings;

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
        SceneContext* __restrict sceneData;
        Random::MersenneTwister* __restrict twister;
        float3* __restrict imageData;

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
}