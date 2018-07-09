
//==============================================================================
// Joe Schutte
//==============================================================================

#include "PathTracer.h"
#include "Shading/Lighting.h"
#include "Shading/SurfaceParameters.h"
#include "Shading/IntegratorContexts.h"
#include "Shading/AreaLighting.h"
#include "SceneLib/SceneResource.h"
#include "SceneLib/ImageBasedLightResource.h"
#include "TextureLib/TextureFiltering.h"
#include "TextureLib/TextureResource.h"
#include "GeometryLib/Camera.h"
#include "GeometryLib/Ray.h"
#include "GeometryLib/SurfaceDifferentials.h"
#include "UtilityLib/FloatingPoint.h"
#include "MathLib/FloatFuncs.h"
#include "MathLib/FloatStructs.h"
#include "MathLib/Trigonometric.h"
#include "MathLib/ImportanceSampling.h"
#include "MathLib/Random.h"
#include "MathLib/Projection.h"
#include "MathLib/Quaternion.h"
#include "ContainersLib/Rect.h"
#include "ThreadingLib/Thread.h"
#include "SystemLib/OSThreading.h"
#include "SystemLib/Atomic.h"
#include "SystemLib/MemoryAllocation.h"
#include "SystemLib/Memory.h"
#include "SystemLib/BasicTypes.h"
#include "SystemLib/MinMax.h"
#include "SystemLib/SystemTime.h"
#include "SystemLib/Logging.h"

#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>

#define MaxBounceCount_         10

#define EnableMultiThreading_   1
#define PathsPerPixel_          8
// -- when zero, PathsPerPixel_ will be used.
#define IntegrationSeconds_     30.0f

namespace Selas
{
    namespace PathTracer
    {
        //==============================================================================
        struct PathTracingKernelData
        {
            SceneContext* sceneData;
            RayCastCameraSettings camera;
            uint width;
            uint height;
            uint pathsPerPixel;
            uint maxBounceCount;
            float integrationSeconds;
            std::chrono::high_resolution_clock::time_point integrationStartTime;

            volatile int64* pathsEvaluatedPerPixel;
            volatile int64* completedThreads;
            volatile int64* kernelIndices;

            Framebuffer* frame;
        };

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
        static bool RayPick(const RTCScene& rtcScene, const Ray& ray, HitParameters& hit)
        {

            RTCIntersectContext context;
            rtcInitIntersectContext(&context);

            Align_(16) RTCRayHit rayhit;
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
            hit.geomId = rayhit.hit.geomID;
            hit.primId = rayhit.hit.primID;
            hit.incDirection = -ray.direction;

            const float kErr = 32.0f * 1.19209e-07f;
            hit.error = kErr * Max(Max(Math::Absf(hit.position.x), Math::Absf(hit.position.y)), Max(Math::Absf(hit.position.z), rayhit.ray.tfar));

            return true;
        }

        //==============================================================================
        static void EvaluateRayBatch(GIIntegrationContext* __restrict context, Ray ray, uint x, uint y)
        {
            float3 throughput = float3::One_;

            uint bounceCount = 0;
            while (bounceCount < context->maxPathLength) {
                
                HitParameters hit;
                bool rayCastHit = RayPick(context->sceneData->rtcScene, ray, hit);

                if(rayCastHit) {
                    SurfaceParameters surface;
                    if(CalculateSurfaceParams(context, &hit, surface) == false) {
                        break;
                    }

                    // -- choose a light and sample the light source
                    LightDirectSample lightSample;
                    DirectIblLightSample(context, lightSample);

                    {
                        float forwardPdfW;
                        float reversePdfW;
                        float3 reflectance = EvaluateBsdf(surface, -ray.direction, lightSample.direction, forwardPdfW, reversePdfW);
                        if(Dot(reflectance, float3::One_) > 0) {

                            if(OcclusionRay(context->sceneData->rtcScene, surface, lightSample.direction, lightSample.distance)) {

                                float3 sample = reflectance * lightSample.radiance * (1.0f / lightSample.directionPdfA);

                                // JSTODO - WARNING! We want directionPdfW not directionPdfA here. However, since we're currently only sampling
                                // IBLs which return the solid angle pdf in directionPdfA we're good for now.
                                float weight = ImportanceSampling::BalanceHeuristic(1, lightSample.directionPdfA, 1, forwardPdfW);
                                FramebufferWriter_Write(&context->frameWriter, weight * throughput * sample, (uint32)x, (uint32)y);
                            }
                        }
                    }

                    {
                        // - sample the bsdf
                        BsdfSample bsdfSample;
                        if(SampleBsdfFunction(context, surface, -ray.direction, bsdfSample) == false) {
                            break;
                        }

                        float lightPdfW = DirectIblLightPdf(context, bsdfSample.wi);
                        float weight = ImportanceSampling::BalanceHeuristic(1, bsdfSample.forwardPdfW, 1, lightPdfW);

                        throughput = weight * throughput * bsdfSample.reflectance;

                        float3 offsetOrigin = OffsetRayOrigin(surface, bsdfSample.wi, 1.0f);
                        ray = MakeRay(offsetOrigin, bsdfSample.wi);
                        ++bounceCount;
                    }
                }
                else {
                    float pdf;
                    float3 sample = SampleIbl(context->sceneData->ibl, ray.direction, pdf);
                    FramebufferWriter_Write(&context->frameWriter, throughput * sample, (uint32)x, (uint32)y);
                    break;
                }
            }
        }

        //==============================================================================
        static void CreatePrimaryRay(GIIntegrationContext* context, uint pixelIndex, uint x, uint y)
        {
            Ray ray = JitteredCameraRay(context->camera, context->twister, (float)x, (float)y);
            EvaluateRayBatch(context, ray, x, y);
        }

        //==============================================================================
        static void PathTracing(GIIntegrationContext* context, uint raysPerPixel, uint width, uint height)
        {
            for(uint y = 0; y < height; ++y) {
                for(uint x = 0; x < width; ++x) {
                    for(uint scan = 0; scan < raysPerPixel; ++scan) {
                        CreatePrimaryRay(context, y * width + x, x, y);
                    }
                }
            }
        }

        //==============================================================================
        static void PathTracerKernel(void* userData)
        {
            PathTracingKernelData* integratorContext = static_cast<PathTracingKernelData*>(userData);
            int64 kernelIndex = Atomic::Increment64(integratorContext->kernelIndices);

            Random::MersenneTwister twister;
            Random::MersenneTwisterInitialize(&twister, (uint32)kernelIndex);

            uint width = integratorContext->width;
            uint height = integratorContext->height;

            float3* imageData = AllocArrayAligned_(float3, width * height, CacheLineSize_);
            Memory::Zero(imageData, sizeof(float3) * width * height);

            GIIntegrationContext context;
            context.sceneData        = integratorContext->sceneData;
            context.camera           = &integratorContext->camera;
            context.twister          = &twister;
            context.maxPathLength    = integratorContext->maxBounceCount;
            FramebufferWriter_Initialize(&context.frameWriter, integratorContext->frame,
                                         DefaultFrameWriterCapacity_, DefaultFrameWriterSoftCapacity_);

            if(integratorContext->integrationSeconds > 0.0f) {

                int64 pathsTracedPerPixel = 0;
                float elapsedSeconds = 0.0f;
                while(elapsedSeconds < integratorContext->integrationSeconds) {
                    PathTracing(&context, 1, width, height);
                    ++pathsTracedPerPixel;

                    elapsedSeconds = SystemTime::ElapsedSecondsF(integratorContext->integrationStartTime);
                }

                Atomic::Add64(integratorContext->pathsEvaluatedPerPixel, pathsTracedPerPixel);

            }
            else {
                PathTracing(&context, integratorContext->pathsPerPixel, width, height);
                Atomic::Add64(integratorContext->pathsEvaluatedPerPixel, integratorContext->pathsPerPixel);
            }

            Random::MersenneTwisterShutdown(&twister);

            FramebufferWriter_Shutdown(&context.frameWriter);
            Atomic::Increment64(integratorContext->completedThreads);
        }

        //==============================================================================
        void GenerateImage(SceneContext& context, cpointer imageName, uint width, uint height)
        {
            const SceneResource* scene = context.scene;
            SceneMetaData* sceneData = scene->data;

            Framebuffer frame;
            FrameBuffer_Initialize(&frame, (uint32)width, (uint32)height);

            RayCastCameraSettings camera;
            InitializeRayCastCamera(scene->data->camera, width, height, camera);

            int64 completedThreads = 0;
            int64 kernelIndex = 0;
            int64 pathsEvaluatedPerPixel = 0;

            #if EnableMultiThreading_ 
                const uint additionalThreadCount = 6;
            #else
                const uint additionalThreadCount = 0;
            #endif
            static_assert(IntegrationSeconds_ != 0.0f || PathsPerPixel_ % (additionalThreadCount + 1) == 0, "Path count not divisible by number of threads");

            PathTracingKernelData integratorContext;
            integratorContext.sceneData              = &context;
            integratorContext.camera                 = camera;
            integratorContext.width                  = width;
            integratorContext.height                 = height;
            integratorContext.maxBounceCount         = MaxBounceCount_;
            integratorContext.pathsPerPixel          = PathsPerPixel_ / (additionalThreadCount + 1);
            integratorContext.integrationStartTime   = SystemTime::Now();
            integratorContext.integrationSeconds     = IntegrationSeconds_;
            integratorContext.pathsEvaluatedPerPixel = &pathsEvaluatedPerPixel;
            integratorContext.completedThreads       = &completedThreads;
            integratorContext.kernelIndices          = &kernelIndex;
            integratorContext.frame                  = &frame;

            #if EnableMultiThreading_
                ThreadHandle threadHandles[additionalThreadCount];

                // -- fork threads
                for(uint scan = 0; scan < additionalThreadCount; ++scan) {
                    threadHandles[scan] = CreateThread(PathTracerKernel, &integratorContext);
                }
            #endif

            // -- do work on the main thread too
            PathTracerKernel(&integratorContext);

            #if EnableMultiThreading_ 
                // -- wait for any other threads to finish
                while(*integratorContext.completedThreads != *integratorContext.kernelIndices);

                for(uint scan = 0; scan < additionalThreadCount; ++scan) {
                    ShutdownThread(threadHandles[scan]);
                }
            #endif

            Logging::WriteDebugInfo("Unidirectional PT integration performed with %lld iterations", pathsEvaluatedPerPixel);
            FrameBuffer_Normalize(&frame, (1.0f / pathsEvaluatedPerPixel));

            FrameBuffer_Save(&frame, imageName);
            FrameBuffer_Shutdown(&frame);
        }
    }
}