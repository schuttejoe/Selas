#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "SceneLib/SceneResource.h"
#include "SceneLib/ImageBasedLightResource.h"
#include "TextureLib/Framebuffer.h"
#include "GeometryLib/Camera.h"
#include "GeometryLib/Ray.h"
#include "MathLib/Random.h"
#include "SystemLib/BasicTypes.h"

struct RTCSceneTy;
typedef struct RTCSceneTy* RTCScene;

namespace Selas
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
        FramebufferWriter                       frameWriter;
        uint                                    imageWidth;
        uint                                    imageHeight;
        uint                                    maxPathLength;
    };

    //==============================================================================
    struct HitParameters
    {
        float3 position;
        float error;
        int32 geomId;
        int32 primId;

        float2 baryCoords;

        float3 rxOrigin;
        float3 rxDirection;
        float3 ryOrigin;
        float3 ryDirection;
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
}