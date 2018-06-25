
//==============================================================================
// Joe Schutte
//==============================================================================

#include "DeferredVCM.h"
#include "VCMCommon.h"

#include "Shading/IntegratorContexts.h"
#include "SceneLib/SceneResource.h"
#include "GeometryLib/Camera.h"

#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>

#define MaxBounceCount_         10

#define EnableMultiThreading_   1
#define IntegrationSeconds_     30.0f

#define VcmRadiusFactor_ 0.0025f
#define VcmRadiusAlpha_ 0.75f

namespace Selas
{
    namespace DeferredVCM
    {
        //==============================================================================
        void GenerateImage(SceneContext& context, Framebuffer* frame)
        {
            //const SceneResource* scene = context.scene;
            //SceneMetaData* sceneData = scene->data;

            //uint width = frame->width;
            //uint height = frame->height;

            //RayCastCameraSettings camera;
            //InitializeRayCastCamera(scene->data->camera, width, height, camera);


        }
    }
}