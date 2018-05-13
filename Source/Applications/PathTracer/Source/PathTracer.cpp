
//==============================================================================
// Joe Schutte
//==============================================================================

#include "PathTracer.h"
#include <Shading/Shading.h>
#include <Shading/SurfaceParameters.h>
#include <Shading/IntegratorContexts.h>

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

#define EnableMultiThreading_   1
#define PathsPerPixel_          8
// -- when zero, PathsPerPixel_ will be used.
#define IntegrationSeconds_     16.0f

namespace Shooty
{
    namespace PathTracer
    {
        //==============================================================================
        struct IntegrationContext
        {
            SceneContext* sceneData;
            RayCastCameraSettings camera;
            uint width;
            uint height;
            uint pathsPerPixel;
            uint maxBounceCount;
            float integrationSeconds;
            int64 integrationStartTime;

            volatile int64* pathsEvaluatedPerPixel;
            volatile int64* completedThreads;
            volatile int64* kernelIndices;

            void* imageDataSpinlock;
            float3* imageData;
        };

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
            hit.rxOrigin = ray.rxOrigin;
            hit.rxDirection = ray.rxDirection;
            hit.ryOrigin = ray.ryOrigin;
            hit.ryDirection = ray.ryDirection;
            hit.pixelIndex = ray.pixelIndex;
            hit.throughput = ray.throughput;
            hit.bounceCount = ray.bounceCount;

            return true;
        }

        //==============================================================================
        static void EvaluateRayBatch(KernelContext* __restrict context)
        {
            while(context->rayStackCount > 0) {

                Ray ray = context->rayStack[context->rayStackCount - 1];
                --context->rayStackCount;

                HitParameters hit;
                bool rayCastHit = RayPick(context->sceneData->rtcScene, ray, hit);

                if(rayCastHit) {
                    ShadeSurfaceHit(context, hit);
                }
                else {
                    float3 sample = SampleIbl(context->sceneData->ibl, ray.direction);
                    AccumulatePixelEnergy(context, ray, sample);
                }
            }
        }

        //==============================================================================
        static void CreatePrimaryRay(KernelContext* context, uint pixelIndex, uint x, uint y)
        {
            Ray ray = JitteredCameraRay(context->camera, context->twister, (uint32)pixelIndex, (float)x, (float)y);
            InsertRay(context, ray);

            EvaluateRayBatch(context);
        }

        //==============================================================================
        static void PathTracing(KernelContext* kernelContext, uint raysPerPixel, uint width, uint height)
        {
            for(uint y = 0; y < height; ++y) {
                for(uint x = 0; x < width; ++x) {
                    for(uint scan = 0; scan < raysPerPixel; ++scan) {
                        CreatePrimaryRay(kernelContext, y * width + x, x, y);
                    }
                }
            }
        }

        //==============================================================================
        static void PathTracerKernel(void* userData)
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
            kernelContext.twister          = &twister;
            kernelContext.maxPathLength   = integratorContext->maxBounceCount;
            kernelContext.rayStackCapacity = 1024 * 1024;
            kernelContext.rayStackCount    = 0;
            kernelContext.rayStack         = AllocArrayAligned_(Ray, kernelContext.rayStackCapacity, CacheLineSize_);

            if(integratorContext->integrationSeconds > 0.0f) {

                int64 pathsTracedPerPixel = 0;
                float elapsedMs = 0.0f;
                while(elapsedMs < integratorContext->integrationSeconds) {
                    PathTracing(&kernelContext, 1, width, height);
                    ++pathsTracedPerPixel;

                    int64 startTime = integratorContext->integrationStartTime;
                    elapsedMs = SystemTime::ElapsedMs(startTime) / 1000.0f;
                }

                Atomic::Add64(integratorContext->pathsEvaluatedPerPixel, pathsTracedPerPixel);

            }
            else {
                PathTracing(&kernelContext, integratorContext->pathsPerPixel, width, height);
                Atomic::Add64(integratorContext->pathsEvaluatedPerPixel, integratorContext->pathsPerPixel);
            }

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
            static_assert(PathsPerPixel_ % (additionalThreadCount + 1) == 0, "Path count not divisible by number of threads");

            IntegrationContext integratorContext;
            integratorContext.sceneData              = &context;
            integratorContext.camera                 = camera;
            integratorContext.imageData              = imageData;
            integratorContext.width                  = width;
            integratorContext.height                 = height;
            integratorContext.maxBounceCount         = MaxBounceCount_;
            integratorContext.pathsPerPixel          = PathsPerPixel_ / (additionalThreadCount + 1);
            SystemTime::GetCycleCounter(&integratorContext.integrationStartTime);
            integratorContext.integrationSeconds     = IntegrationSeconds_;
            integratorContext.pathsEvaluatedPerPixel = &pathsEvaluatedPerPixel;
            integratorContext.completedThreads       = &completedThreads;
            integratorContext.kernelIndices          = &kernelIndex;
            integratorContext.imageDataSpinlock      = CreateSpinLock();

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