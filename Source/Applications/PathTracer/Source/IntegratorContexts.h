#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SceneLib/SceneResource.h>
#include <SceneLib/ImageBasedLightResource.h>
#include <GeometryLib/Camera.h>
#include <MathLib/Random.h>

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
        SceneResource* scene;
        ImageBasedLightResourceData* ibl;
    };

    //==============================================================================
    struct KernelContext
    {
        const SceneContext* sceneData;
        const RayCastCameraSettings* camera;
        Random::MersenneTwister* twister;
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
        float ior;
    };
}