
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
#include "GeometryLib/HashGrid.h"
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
        static bool OcclusionRay(RTCScene& rtcScene, const SurfaceParameters& surface, float3 direction, float distance)
        {
            float3 origin = OffsetRayOrigin(surface, direction, 0.1f);

            RTCIntersectContext context;
            rtcInitIntersectContext(&context);

            Align_(16) RTCRay ray;
            ray.org_x = origin.x;
            ray.org_y = origin.y;
            ray.org_z = origin.z;
            ray.dir_x = direction.x;
            ray.dir_y = direction.y;
            ray.dir_z = direction.z;
            ray.tnear = surface.error;
            ray.tfar = distance;

            rtcOccluded1(rtcScene, &context, &ray);

            // -- ray.tfar == -inf when hit occurs
            return (ray.tfar >= 0.0f);
        }

        //==============================================================================
        static bool VcOcclusionRay(RTCScene& rtcScene, const SurfaceParameters& surface, float3 direction, float distance)
        {
            float biasDistance;
            float3 origin = OffsetRayOrigin(surface, direction, 0.1f, biasDistance);

            RTCIntersectContext context;
            rtcInitIntersectContext(&context);

            Align_(16) RTCRay ray;
            ray.org_x = origin.x;
            ray.org_y = origin.y;
            ray.org_z = origin.z;
            ray.dir_x = direction.x;
            ray.dir_y = direction.y;
            ray.dir_z = direction.z;
            ray.tnear = surface.error;
            ray.tfar = distance - 16.0f * Math::Absf(biasDistance);

            rtcOccluded1(rtcScene, &context, &ray);

            // -- ray.tfar == -inf when hit occurs
            return (ray.tfar >= 0.0f);
        }

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
        float3 ConnectToSkyLight(GIIntegrationContext* context, PathState& state)
        {
            float directPdfA;
            float emissionPdfW;
            float3 radiance = IblCalculateRadiance(context, state.direction, directPdfA, emissionPdfW);

            if(state.pathLength == 1) {
                return radiance;
            }

            float cameraWeight = directPdfA * state.dVCM + emissionPdfW * state.dVC;
            float misWeight = 1.0f / (1.0f + cameraWeight);

            return misWeight * radiance;
        }

        //==============================================================================
        void ConnectLightPathToCamera(GIIntegrationContext* context, PathState& state, const SurfaceParameters& surface, float vmWeight, float lightPathCount)
        {
            const RayCastCameraSettings* __restrict camera = context->camera;

            float3 toPosition = surface.position - camera->position;
            if(Dot(camera->forward, toPosition) <= 0.0f) {
                return;
            }

            int2 imagePosition = WorldToImage(camera, surface.position);
            if(imagePosition.x < 0 || imagePosition.x >= camera->viewportWidth || imagePosition.y < 0 || imagePosition.y >= camera->viewportHeight) {
                return;
            }

            float distance = Length(toPosition);
            toPosition = (1.0f / distance) * toPosition;

            // -- evaluate BSDF
            float bsdfForwardPdf;
            float bsdfReversePdf;
            float3 bsdf = EvaluateBsdf(surface, -state.direction, -toPosition, bsdfForwardPdf, bsdfReversePdf);
            if(bsdf.x == 0 && bsdf.y == 0 && bsdf.z == 0) {
                return;
            }

            float cosThetaSurface = Math::Absf(Dot(surface.geometricNormal, -toPosition));
            float cosThetaCamera  = Dot(camera->forward, toPosition);

            float imagePointToCameraDistance = camera->virtualImagePlaneDistance / cosThetaCamera;
            float imageToSolidAngle = imagePointToCameraDistance * imagePointToCameraDistance / cosThetaCamera;
            float imageToSurface = imageToSolidAngle * cosThetaCamera;
            float surfaceToImage = 1.0f / imageToSurface;
            float cameraPdfA = imageToSurface;

            float lightPartialWeight = (cameraPdfA / lightPathCount) * (vmWeight + state.dVCM + state.dVC * bsdfReversePdf);
            float misWeight = 1.0f / (lightPartialWeight + 1.0f);
            
            float3 pathContribution = misWeight * state.throughput * bsdf * (1.0f / (lightPathCount * surfaceToImage));
            if(pathContribution.x == 0 && pathContribution.y == 0 && pathContribution.z == 0) {
                return;
            }

            if(OcclusionRay(context->sceneData->rtcScene, surface, -toPosition, distance)) {
                FramebufferWriter_Write(&context->frameWriter, pathContribution, imagePosition.x, imagePosition.y);
            }
        }

        //==============================================================================
        float3 ConnectCameraPathToLight(GIIntegrationContext* context, PathState& state, const SurfaceParameters& surface, float vmWeight)
        {
            // -- only using the ibl for now
            float lightSampleWeight = 1.0f;

            // -- choose direction to sample the ibl
            float r0 = Random::MersenneTwisterFloat(context->twister);
            float r1 = Random::MersenneTwisterFloat(context->twister);

            LightDirectSample sample;
            {
                // -- JSTODO - Sample area lights and such
                DirectIblLightSample(context, sample);
                sample.directionPdfA *= lightSampleWeight;
            }
            
            float bsdfForwardPdfW;
            float bsdfReversePdfW;
            float3 bsdf = EvaluateBsdf(surface, -state.direction, sample.direction, bsdfForwardPdfW, bsdfReversePdfW);
            if(bsdf.x == 0 && bsdf.y == 0 && bsdf.z == 0) {
                return float3::Zero_;
            }

            float cosThetaSurface = Math::Absf(Dot(surface.perturbedNormal, sample.direction));

            float lightWeight = bsdfForwardPdfW / sample.directionPdfA;
            float cameraWeight = (sample.emissionPdfW * cosThetaSurface / (sample.directionPdfA * sample.cosThetaLight)) * (vmWeight + state.dVCM + state.dVC * bsdfReversePdfW);
            float misWeight = 1.0f / (lightWeight + 1 + cameraWeight);

            float3 pathContribution = (misWeight * cosThetaSurface / sample.directionPdfA) * sample.radiance * bsdf;
            if(pathContribution.x == 0 && pathContribution.y == 0 && pathContribution.z == 0) {
                return float3::Zero_;
            }

            if(OcclusionRay(context->sceneData->rtcScene, surface, sample.direction, sample.distance)) {
                return pathContribution;
            }

            return float3::Zero_;
        }

        //==============================================================================
        float3 ConnectPathVertices(GIIntegrationContext* context, const SurfaceParameters& surface, const PathState& cameraState, const VcmVertex& lightVertex, float vmWeight)
        {
            float3 direction = lightVertex.surface.position - surface.position;
            float distanceSquared = LengthSquared(direction);
            float distance = Math::Sqrtf(distanceSquared);
            direction = (1.0f / distance) * direction;

            float cameraBsdfForwardPdfW;
            float cameraBsdfReversePdfW;
            float3 cameraBsdf = EvaluateBsdf(surface, -cameraState.direction, direction, cameraBsdfForwardPdfW, cameraBsdfReversePdfW);
            if(cameraBsdf.x == 0 && cameraBsdf.y == 0 && cameraBsdf.z == 0) {
                return float3::Zero_;
            }

            float lightBsdfForwardPdfW;
            float lightBsdfReversePdfW;
            float3 lightBsdf = EvaluateBsdf(lightVertex.surface, -direction, lightVertex.surface.view, lightBsdfForwardPdfW, lightBsdfReversePdfW);
            if(lightBsdf.x == 0 && lightBsdf.y == 0 && lightBsdf.z == 0) {
                return float3::Zero_;
            }

            float cosThetaCamera = Math::Absf(Dot(direction, surface.perturbedNormal));
            float cosThetaLight = Math::Absf(Dot(-direction, lightVertex.surface.perturbedNormal));

            float geometryTerm = cosThetaLight * cosThetaCamera / distanceSquared;
            if(geometryTerm < 0.0f) {
                // -- JSTODO - For this to be possible the cosTheta terms would need to be negative. But with transparent surfaces the normal will often be for the other side of the surface.
                return float3::Zero_;
            }

            // -- convert pdfs from solid angle to area measure
            float cameraBsdfPdfA = cameraBsdfForwardPdfW * Math::Absf(cosThetaLight) / distanceSquared;
            float lightBsdfPdfA = lightBsdfForwardPdfW * Math::Absf(cosThetaCamera) / distanceSquared;
            float lightWeight = cameraBsdfPdfA * (vmWeight + lightVertex.dVCM + lightVertex.dVC * lightBsdfReversePdfW);
            float cameraWeight = lightBsdfPdfA * (vmWeight + cameraState.dVCM + cameraState.dVC * cameraBsdfReversePdfW);
            float misWeight = 1.0f / (lightWeight + 1.0f + cameraWeight);

            float3 pathContribution = misWeight * geometryTerm * cameraBsdf * lightBsdf;
            if(pathContribution.x == 0 && pathContribution.y == 0 && pathContribution.z == 0) {
                return float3::Zero_;
            }

            if(VcOcclusionRay(context->sceneData->rtcScene, surface, direction, distance)) {
                return pathContribution;
            }

            return float3::Zero_;
        }

        //==============================================================================
        void MergeVertices(uint vertexIndex, void* userData)
        {
            VertexMergingCallbackStruct* vmData = (VertexMergingCallbackStruct*)userData;
            const GIIntegrationContext* context = vmData->context;
            const SurfaceParameters& surface = *vmData->surface;
            const PathState& cameraState = *vmData->cameraState;
            const VcmVertex& lightVertex = vmData->pathVertices->GetData()[vertexIndex];

            if(cameraState.pathLength + lightVertex.pathLength > context->maxPathLength) {
                return;
            }

            // JSTODO - This is a hack :(. How do I correctly prevent ringing around the base of glossy transparent objects?
            if((surface.materialFlags & eTransparent) != (lightVertex.surface.materialFlags & eTransparent)) {
                return;
            }

            float bsdfForwardPdfW;
            float bsdfReversePdfW;
            float3 bsdf = EvaluateBsdf(surface, -cameraState.direction, lightVertex.surface.view, bsdfForwardPdfW, bsdfReversePdfW);
            if(bsdf.x == 0 && bsdf.y == 0 && bsdf.z == 0) {
                return;
            }

            float lightWeight = lightVertex.dVCM * vmData->vcWeight + lightVertex.dVM * bsdfForwardPdfW;
            float cameraWeight = cameraState.dVCM * vmData->vcWeight + cameraState.dVM * bsdfReversePdfW;
            float misWeight = 1.0f / (lightWeight + 1.0f + cameraWeight);

            #if CheckForNaNs_
                Assert_(!Math::IsNaN(bsdf.x));
                Assert_(!Math::IsNaN(bsdf.y));
                Assert_(!Math::IsNaN(bsdf.z));
                Assert_(!Math::IsNaN(lightVertex.throughput.x));
                Assert_(!Math::IsNaN(lightVertex.throughput.y));
                Assert_(!Math::IsNaN(lightVertex.throughput.z));
            #endif

            vmData->result += misWeight * bsdf * lightVertex.throughput;
        }

        //==============================================================================
        bool SampleBsdfScattering(GIIntegrationContext* context, const SurfaceParameters& surface, float vmWeight, float vcWeight, PathState& pathState)
        {
            BsdfSample sample;
            if(SampleBsdfFunction(context, surface, -pathState.direction, sample) == false) {
                return false;
            }
            if(sample.reflectance.x == 0.0f && sample.reflectance.y == 0.0f && sample.reflectance.z == 0.0f) {
                return false;
            }

            float cosThetaBsdf = Math::Absf(Dot(sample.wi, surface.perturbedNormal));

            pathState.position = surface.position;
            pathState.throughput = pathState.throughput * sample.reflectance;
            pathState.dVC = (cosThetaBsdf / sample.forwardPdfW) * (pathState.dVC * sample.reversePdfW + pathState.dVCM + vmWeight);
            pathState.dVM = (cosThetaBsdf / sample.forwardPdfW) * (pathState.dVM * sample.reversePdfW + pathState.dVCM * vcWeight + 1.0f);
            pathState.dVCM = 1.0f / sample.forwardPdfW;
            pathState.direction = sample.wi;
            ++pathState.pathLength;

            return true;
        }
    }
}