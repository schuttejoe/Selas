
//==============================================================================
// Joe Schutte
//==============================================================================

#include "VCM.h"
#include <Shading/Shading.h>
#include <Shading/SurfaceParameters.h>
#include <Shading/IntegratorContexts.h>
#include <Shading/AreaLighting.h>

#include <SceneLib/SceneResource.h>
#include <SceneLib/ImageBasedLightResource.h>
#include <TextureLib/TextureFiltering.h>
#include <TextureLib/TextureResource.h>
#include <GeometryLib/Camera.h>
#include <GeometryLib/Ray.h>
#include <GeometryLib/SurfaceDifferentials.h>
#include <UtilityLib/FloatingPoint.h>
#include <MathLib/FloatFuncs.h>
#include <MathLib/FloatStructs.h>
#include <MathLib/IntStructs.h>
#include <MathLib/Trigonometric.h>
#include <MathLib/ImportanceSampling.h>
#include <MathLib/Random.h>
#include <MathLib/Projection.h>
#include <MathLib/Quaternion.h>
#include <ContainersLib/Rect.h>
#include <ThreadingLib/Thread.h>
#include <SystemLib/OSThreading.h>
#include <SystemLib/Atomic.h>
#include <SystemLib/MemoryAllocation.h>
#include <SystemLib/Memory.h>
#include <SystemLib/BasicTypes.h>
#include <SystemLib/MinMax.h>
#include <SystemLib/SystemTime.h>

#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>

#define MaxBounceCount_         10

#define EnableMultiThreading_   0
#define IntegrationSeconds_     30.0f

#define VcmRadiusFactor_ 0.005f
#define VcmRadiusAlpha_ 0.75f

namespace Shooty
{
    namespace VCM
    {
        //==============================================================================
        struct IntegrationContext
        {
            SceneContext* sceneData;
            RayCastCameraSettings camera;
            uint width;
            uint height;
            uint maxBounceCount;
            float integrationSeconds;
            int64 integrationStartTime;

            float vcmRadius;
            float vcmRadiusAlpha;

            volatile int64* pathsEvaluatedPerPixel;
            volatile int64* completedThreads;
            volatile int64* kernelIndices;

            void* imageDataSpinlock;
            float3* imageData;
        };

        //==============================================================================
        bool OcclusionRay(RTCScene& rtcScene, const SurfaceParameters& surface, float3 direction, float distance)
        {
            float3 origin = OffsetRayOrigin(surface, direction, 4.0f);

            RTCIntersectContext context;
            rtcInitIntersectContext(&context);

            __declspec(align(16)) RTCRay ray;
            ray.org_x = origin.x;
            ray.org_y = origin.y;
            ray.org_z = origin.z;
            ray.dir_x = direction.x;
            ray.dir_y = direction.y;
            ray.dir_z = direction.z;
            ray.tnear = surface.error;
            ray.tfar  = distance;

            rtcOccluded1(rtcScene, &context, &ray);

            // -- ray.tfar == -inf when hit occurs
            return (ray.tfar >= 0.0f);
        }

        //==============================================================================
        static bool RayPick(const RTCScene& rtcScene, const Ray& ray, HitParameters& hit)
        {

            RTCIntersectContext context;
            rtcInitIntersectContext(&context);

            __declspec(align(16)) RTCRayHit rayhit;
            rayhit.ray.org_x = ray.origin.x;
            rayhit.ray.org_y = ray.origin.y;
            rayhit.ray.org_z = ray.origin.z;
            rayhit.ray.dir_x = ray.direction.x;
            rayhit.ray.dir_y = ray.direction.y;
            rayhit.ray.dir_z = ray.direction.z;
            rayhit.ray.tnear = 0.00001f;
            rayhit.ray.tfar = FloatMax_;

            rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
            rayhit.hit.primID = RTC_INVALID_GEOMETRY_ID;

            rtcIntersect1(rtcScene, &context, &rayhit);

            if(rayhit.hit.geomID == -1)
                return false;

            hit.position.x = rayhit.ray.org_x + rayhit.ray.tfar * ray.direction.x;
            hit.position.y = rayhit.ray.org_y + rayhit.ray.tfar * ray.direction.y;
            hit.position.z = rayhit.ray.org_z + rayhit.ray.tfar * ray.direction.z;
            hit.baryCoords = { rayhit.hit.u, rayhit.hit.v };
            hit.primId = rayhit.hit.primID;

            const float kErr = 32.0f * 1.19209e-07f;
            hit.error = kErr * Max(Max(Math::Absf(hit.position.x), Math::Absf(hit.position.y)), Max(Math::Absf(hit.position.z), rayhit.ray.tfar));

            hit.viewDirection = -ray.direction;
            hit.rxOrigin      = ray.rxOrigin;
            hit.rxDirection   = ray.rxDirection;
            hit.ryOrigin      = ray.ryOrigin;
            hit.ryDirection   = ray.ryDirection;
            hit.pixelIndex    = ray.pixelIndex;
            hit.throughput    = ray.throughput;
            hit.bounceCount   = ray.bounceCount;

            return true;
        }

        struct PathState
        {
            float3 position;
            float3 direction;
            float3 throughput;
            float dVCM;
            float dVC;
            float dVM;
            uint32 pathLength    : 31;
            uint32 isAreaMeasure : 1;
        };

        //==============================================================================
        static void GenerateLightSample(KernelContext* context, float vcWeight, PathState& state)
        {
            // -- right now we're just generating a sample on the ibl
            float lightSampleWeight = 1.0f;

            LightSample sample;
            GenerateIblLightSample(context, sample);

            sample.emissionPdfW  *= lightSampleWeight;
            sample.directionPdfA *= lightSampleWeight;

            state.position        = sample.position;
            state.direction       = sample.direction;
            state.throughput      = sample.radiance * (1.0f / sample.emissionPdfW);
            state.dVCM            = sample.directionPdfA / sample.emissionPdfW;
            state.dVC             = sample.cosThetaLight / sample.emissionPdfW;
            state.dVM             = state.dVC * vcWeight;
            state.pathLength      = 1;
            state.isAreaMeasure   = 0; // -- this would be true for any non infinite light source. false since for ibl-only.
        }

        //==============================================================================
        static void ConnectLightPathToCamera(KernelContext* context, PathState& state, const SurfaceParameters& surface, float vcWeight, float lightPathCount)
        {
            const RayCastCameraSettings* __restrict camera = context->camera;

            float3 toPosition = surface.position - camera->position;
            if(Dot(camera->forward, toPosition) <= 0.0f) {
                return;
            }

            int2 imagePosition = WorldToImage(camera, surface.position);
            if(imagePosition.x < 0.0f || imagePosition.x >= camera->viewportWidth || imagePosition.y < 0.0f || imagePosition.y >= camera->viewportHeight) {
                return;
            }

            float distance = Length(toPosition);
            toPosition = (1.0f / distance) * toPosition;

            // -- evaluate BSDF
            float bsdfPdf;
            float3 bsdf = EvaluateBsdf(surface, -state.direction, -toPosition, bsdfPdf);
            if(bsdf.x == 0 && bsdf.y == 0 && bsdf.z == 0) {
                return;
            }

            float cosThetaSurface = Math::Absf(Dot(surface.perturbedNormal, -toPosition));
            float cosThetaCamera  = Dot(camera->forward, toPosition);

            float imagePointToCameraDistance = camera->imagePlaneDistance / cosThetaCamera;
            float imageToSolidAngle = imagePointToCameraDistance * imagePointToCameraDistance / cosThetaCamera;
            float imageToSurface = imageToSolidAngle * cosThetaCamera;
            float surfaceToImage = 1.0f / imageToSurface;

            float cameraPdfA = imageToSurface;

            float lightPartialWeight = (cameraPdfA / lightPathCount) * (vcWeight + state.dVCM + state.dVC * bsdfPdf);
            float pathWeight = 1.0f / (lightPartialWeight + 1.0f);
            
            float3 sample = pathWeight * state.throughput * bsdf * (1.0f / (lightPathCount * surfaceToImage));
            if(sample.x == 0 && sample.y == 0 && sample.z == 0) {
                return;
            }

            if(OcclusionRay(context->sceneData->rtcScene, surface, -toPosition, distance)) {
                uint index = imagePosition.y * context->imageWidth + imagePosition.x;
                context->imageData[index] = context->imageData[index] + sample;
            }
        }

        //==============================================================================
        static void VertexConnectionAndMerging(KernelContext* context, float kernelRadius, uint width, uint height)
        {
            uint lightPathCount = width * height;

            float vcWeight = Math::Pi_ * kernelRadius * kernelRadius * lightPathCount;
            float vmWeight = 1.0f / vcWeight;

            // -- generate light paths
            for(uint scan = 0; scan < lightPathCount; ++scan) {
                
                // -- create initial light path vertex y_0 
                PathState state;
                GenerateLightSample(context, vcWeight, state);

                while(state.pathLength + 2 < context->maxPathLength) {

                    // -- Make a basic ray. No differentials are used for light path vertices. // JSTODO - Ray storing a pixel index and bounce count was not very forward thinking. Remove those.
                    Ray ray = MakeRay(state.position, state.direction, state.throughput, 0, 0);

                    // -- Cast the ray against the scene
                    HitParameters hit;
                    if(RayPick(context->sceneData->rtcScene, ray, hit) == false) {
                        break;
                    }

                    // -- Calculate all surface information for this hit position
                    SurfaceParameters surface;
                    if(CalculateSurfaceParams(context, &hit, surface) == false) {
                        break;
                    }

                    float connectionLengthSqr = LengthSquared(state.position - surface.position);
                    float absDotNL = Math::Absf(Dot(surface.perturbedNormal, hit.viewDirection));

                    // -- Update accumulated MIS parameters with info from our new hit position
                    if(state.pathLength > 1 || state.isAreaMeasure) {
                        state.dVCM *= connectionLengthSqr;
                    }
                    state.dVCM *= (1.0f / absDotNL);
                    state.dVC  *= (1.0f / absDotNL);
                    state.dVM  *= (1.0f / absDotNL);

                    // -- JSTODO - store the vertex for use with vertex merging

                    // -- connect the path to the camera
                    ConnectLightPathToCamera(context, state, surface, vcWeight, (float)lightPathCount);

                    // -- JSTODO - bsdf scattering to advance the path
                    break;
                }
            }
        }

        //==============================================================================
        static void VCMKernel(void* userData)
        {
            IntegrationContext* integratorContext = static_cast<IntegrationContext*>(userData);
            int64 kernelIndex = Atomic::Increment64(integratorContext->kernelIndices);

            Random::MersenneTwister twister;
            Random::MersenneTwisterInitialize(&twister, (uint32)kernelIndex);

            uint width = integratorContext->width;
            uint height = integratorContext->height;

            float3* imageData = AllocArrayAligned_(float3, width * height, CacheLineSize_);
            Memory::Zero(imageData, sizeof(float3) * width * height);

            KernelContext kernelContext;
            kernelContext.sceneData        = integratorContext->sceneData;
            kernelContext.camera           = &integratorContext->camera;
            kernelContext.imageData        = imageData;
            kernelContext.imageWidth       = width;
            kernelContext.imageHeight      = height;
            kernelContext.twister          = &twister;
            kernelContext.maxPathLength    = integratorContext->maxBounceCount;
            kernelContext.rayStackCapacity = 1024 * 1024;
            kernelContext.rayStackCount    = 0;
            kernelContext.rayStack         = AllocArrayAligned_(Ray, kernelContext.rayStackCapacity, CacheLineSize_);

            float iterationIndex = 0.0f;

            int64 pathsTracedPerPixel = 0;
            float elapsedMs = 0.0f;
            while(elapsedMs < integratorContext->integrationSeconds) {
                iterationIndex += 1.0f;;

                float vcmKernelRadius = integratorContext->vcmRadius / Math::Powf(iterationIndex, 0.5f * (1.0f - integratorContext->vcmRadiusAlpha));

                VertexConnectionAndMerging(&kernelContext, vcmKernelRadius, width, height);
                ++pathsTracedPerPixel;

                int64 startTime = integratorContext->integrationStartTime;
                elapsedMs = SystemTime::ElapsedMs(startTime) / 1000.0f;
            }

            Atomic::Add64(integratorContext->pathsEvaluatedPerPixel, pathsTracedPerPixel);

            FreeAligned_(kernelContext.rayStack);
            Random::MersenneTwisterShutdown(&twister);

            EnterSpinLock(integratorContext->imageDataSpinlock);

            float3* resultImageData = integratorContext->imageData;
            for(uint y = 0; y < height; ++y) {
                for(uint x = 0; x < width; ++x) {
                    uint index = y * width + x;
                    resultImageData[index] += imageData[index];
                }
            }

            LeaveSpinLock(integratorContext->imageDataSpinlock);

            FreeAligned_(imageData);
            Atomic::Increment64(integratorContext->completedThreads);
        }

        //==============================================================================
        void GenerateImage(SceneContext& context, uint width, uint height, float3* imageData)
        {
            const SceneResource* scene = context.scene;
            SceneResourceData* sceneData = scene->data;

            RayCastCameraSettings camera;
            InitializeRayCastCamera(scene->data->camera, width, height, camera);

            int64 completedThreads = 0;
            int64 kernelIndex = 0;
            int64 pathsEvaluatedPerPixel = 0;

            #if EnableMultiThreading_ 
                const uint additionalThreadCount = 7;
            #else
                const uint additionalThreadCount = 0;
            #endif

            IntegrationContext integratorContext;
            integratorContext.sceneData              = &context;
            integratorContext.camera                 = camera;
            integratorContext.imageData              = imageData;
            integratorContext.width                  = width;
            integratorContext.height                 = height;
            integratorContext.maxBounceCount         = MaxBounceCount_;
            SystemTime::GetCycleCounter(&integratorContext.integrationStartTime);
            integratorContext.integrationSeconds     = IntegrationSeconds_;
            integratorContext.pathsEvaluatedPerPixel = &pathsEvaluatedPerPixel;
            integratorContext.completedThreads       = &completedThreads;
            integratorContext.kernelIndices          = &kernelIndex;
            integratorContext.imageDataSpinlock      = CreateSpinLock();
            integratorContext.vcmRadius              = VcmRadiusFactor_ * sceneData->boundingSphere.w;
            integratorContext.vcmRadiusAlpha         = VcmRadiusAlpha_;

            #if EnableMultiThreading_
                ThreadHandle threadHandles[additionalThreadCount];

                // -- fork threads
                for(uint scan = 0; scan < additionalThreadCount; ++scan) {
                    threadHandles[scan] = CreateThread(VCMKernel, &integratorContext);
                }
            #endif

            // -- do work on the main thread too
            VCMKernel(&integratorContext);

            #if EnableMultiThreading_ 
                // -- wait for any other threads to finish
                while(*integratorContext.completedThreads != *integratorContext.kernelIndices);

                for(uint scan = 0; scan < additionalThreadCount; ++scan) {
                    ShutdownThread(threadHandles[scan]);
                }
            #endif

            CloseSpinlock(integratorContext.imageDataSpinlock);

            for(uint y = 0; y < height; ++y) {
                for(uint x = 0; x < width; ++x) {
                    uint index = y * width + x;
                    imageData[index] = imageData[index] * (1.0f / pathsEvaluatedPerPixel);
                }
            }
        }
    }
}