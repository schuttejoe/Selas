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

#define MaxInstanceLevelCount_ 2

namespace Selas
{
    struct SceneResource;
    struct ImageBasedLightResource;
    struct RayCastCameraSettings;
    struct SurfaceParameters;
    class GeometryCache;
    class TextureCache;

    //=============================================================================================================================
    struct GIIntegratorContext
    {
        RTCScene                                rtcScene;

        const SceneResource*                    scene;
        GeometryCache*                          geometryCache;
        TextureCache*                           textureCache;
        const RayCastCameraSettings* __restrict camera;
        CSampler                                sampler;
        FramebufferWriter                       frameWriter;
        uint                                    maxPathLength;
    };

    //=============================================================================================================================
    struct HitParameters
    {
        float3 position;
        float3 normal;
        float3 view;
        float3 throughput;
        float error;
        int32 geomId;
        int32 primId;
        int32 instId[MaxInstanceLevelCount_];
        uint32 index            : 30;
        uint32 diracScatterOnly : 31;
        float2 baryCoords;
    };

    // -- generation of differential rays
    Ray CreateReflectionBounceRay(const SurfaceParameters& surface, const HitParameters& hit, float3 wi, float3 reflectance);
    Ray CreateRefractionBounceRay(const SurfaceParameters& surface, const HitParameters& hit, float3 wi, float3 reflectance, float iorRatio);
}