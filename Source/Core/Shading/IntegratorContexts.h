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
    struct GIIntegrationContext
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
        float3 incDirection; // points in the direction the path that hit this position came from
        float error;
        int32 geomId;
        int32 primId;

        float2 baryCoords;
    };

    struct PathState
    {
        float3 position;
        float3 direction;  // JSTODO - convert to octrahedral format
        float3 throughput; // JSTODO - Testing convert to RGB9e5? Or even half format?
        float dVCM;        // JSTODO - Test as half format
        float dVC;         // JSTODO - Test as half format
        float dVM;         // JSTODO - Test as half format
        uint32 pathLength : 31;
        uint32 isAreaMeasure : 1;
    };

    // -- generation of differential rays
    Ray CreateReflectionBounceRay(const SurfaceParameters& surface, const HitParameters& hit, float3 wi, float3 reflectance);
    Ray CreateRefractionBounceRay(const SurfaceParameters& surface, const HitParameters& hit, float3 wi, float3 reflectance, float iorRatio);
}