#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SceneLib/ModelResource.h"
#include "SceneLib/ImageBasedLightResource.h"
#include "TextureLib/Framebuffer.h"
#include "GeometryLib/Camera.h"
#include "GeometryLib/Ray.h"
#include "MathLib/Sampler.h"
#include "SystemLib/BasicTypes.h"

struct RTCSceneTy;
typedef struct RTCSceneTy* RTCScene;

namespace Selas
{
    struct ModelResource;
    struct ImageBasedLightResource;
    struct RayCastCameraSettings;
    struct SurfaceParameters;

    //=============================================================================================================================
    // JSTODO - This is just an unnecessary indirection. Clean this up when you clean GIIntegrationContext
    struct SceneContext
    {
        RTCScene rtcScene;
        const ModelResource* scene;
        const ImageBasedLightResourceData* ibl;
    };

    //=============================================================================================================================
    struct GIIntegrationContext
    {
        const RayCastCameraSettings* __restrict camera;
        SceneContext*                __restrict sceneData;
        CSampler                                sampler;
        FramebufferWriter                       frameWriter;
        uint                                    imageWidth;
        uint                                    imageHeight;
        uint                                    maxPathLength;
    };

    //=============================================================================================================================
    struct HitParameters
    {
        float3 position;
        float3 incDirection; // points in the direction the path that hit this position came from
        float error;
        int32 geomId;
        int32 primId;

        float2 baryCoords;
    };

    // -- generation of differential rays
    Ray CreateReflectionBounceRay(const SurfaceParameters& surface, const HitParameters& hit, float3 wi, float3 reflectance);
    Ray CreateRefractionBounceRay(const SurfaceParameters& surface, const HitParameters& hit, float3 wi, float3 reflectance, float iorRatio);
}