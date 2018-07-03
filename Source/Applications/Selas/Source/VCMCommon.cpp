
//==============================================================================
// Joe Schutte
//==============================================================================

#include "VCMCommon.h"

#include "Shading/Lighting.h"
#include "Shading/SurfaceParameters.h"
#include "Shading/IntegratorContexts.h"
#include "Shading/AreaLighting.h"
#include "TextureLib/Framebuffer.h"
#include "GeometryLib/Camera.h"
#include "GeometryLib/Ray.h"
#include "ThreadingLib/Thread.h"
#include "SystemLib/OSThreading.h"
#include "SystemLib/Atomic.h"
#include "SystemLib/MinMax.h"
#include "SystemLib/SystemTime.h"

#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>

#define MaxBounceCount_         10

#define EnableMultiThreading_   1
#define IntegrationSeconds_     30.0f

#define VcmRadiusFactor_ 0.0025f
#define VcmRadiusAlpha_ 0.75f

namespace Selas
{
    namespace VCMCommon
    {
        //==============================================================================
        void GenerateLightSample(GIIntegrationContext* context, float vcWeight, PathState& state)
        {
            // -- right now we're just generating a sample on the ibl
            float lightSampleWeight = 1.0f;

            LightEmissionSample sample;
            {
                // -- JSTODO - Sample area lights and such
                EmitIblLightSample(context, sample);
            }

            sample.emissionPdfW  *= lightSampleWeight;
            sample.directionPdfA *= lightSampleWeight;

            state.position        = sample.position;
            state.direction       = sample.direction;
            state.throughput      = sample.radiance * (1.0f / sample.emissionPdfW);
            state.dVCM            = sample.directionPdfA / sample.emissionPdfW;
            state.dVC             = sample.cosThetaLight / sample.emissionPdfW;
            state.dVM             = sample.cosThetaLight / sample.emissionPdfW * vcWeight;
            state.pathLength      = 1;
            state.isAreaMeasure   = 0; // -- this would be true for any non infinite light source. false here since we only sample the ibl.
        }

        //==============================================================================
        void GenerateCameraSample(GIIntegrationContext* context, uint x, uint y, float lightPathCount, PathState& state)
        {
            const RayCastCameraSettings* __restrict camera = context->camera;

            Ray cameraRay = JitteredCameraRay(camera, context->twister, (float)x, (float)y);

            float cosThetaCamera = Dot(camera->forward, cameraRay.direction);
            float imagePointToCameraDistance = camera->virtualImagePlaneDistance / cosThetaCamera;
            float invSolidAngleMeasure = imagePointToCameraDistance * imagePointToCameraDistance / cosThetaCamera;
            float revCameraPdfW = (1.0f / invSolidAngleMeasure);

            state.position      = cameraRay.origin;
            state.direction     = cameraRay.direction;
            state.throughput    = float3::One_;
            state.dVCM          = lightPathCount * revCameraPdfW;
            state.dVC           = 0;
            state.dVM           = 0;
            state.pathLength    = 1;
            state.isAreaMeasure = 1;
        }

        //==============================================================================
        float SearchRadius(float baseRadius, float radiusAlpha, float iterationIndex)
        {
            return baseRadius / Math::Powf(iterationIndex, 0.5f * (1.0f - radiusAlpha));
        }
    }
}