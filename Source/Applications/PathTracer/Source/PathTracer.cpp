
//==============================================================================
// Joe Schutte
//==============================================================================

#include "PathTracer.h"
#include "PathTracerShading.h"
#include "SurfaceParameters.h"
#include "IntegratorContexts.h"

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

#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>

#define SampleSpecificTexel_    0 && Debug_
#define SpecificTexelX_         1196
#define SpecificTexelY_         354

#if SampleSpecificTexel_
#define EnableMultiThreading_   0
#define RaysPerPixel_           1
#else
#define EnableMultiThreading_   1
#define RaysPerPixel_           64
#endif

namespace Shooty
{
    //==============================================================================
    struct PathTracerContext
    {
        SceneContext* sceneData;
        RayCastCameraSettings camera;
        uint blockDimensions;
        uint blockCountX;
        uint blockCountY;
        uint blockCount;
        uint width;
        uint height;
        volatile int64* consumedBlocks;
        volatile int64* completedBlocks;

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
        rayhit.ray.tfar  = FloatMax_;

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
    static void RayCastImageBlock(const PathTracerContext* integratorContext, KernelContext* kernelContext, uint blockIndex)
    {
        Random::MersenneTwisterReseed(kernelContext->twister, (uint32)blockIndex);

        uint width = integratorContext->width;
        uint height = integratorContext->height;

        const RayCastCameraSettings& camera = integratorContext->camera;
        uint blockDimensions = integratorContext->blockDimensions;

        uint by = blockIndex / integratorContext->blockCountX;
        uint bx = blockIndex % integratorContext->blockCountX;

        const uint raysPerPixel = RaysPerPixel_;

        for(uint dy = 0; dy < blockDimensions; ++dy) {
            for(uint dx = 0; dx < blockDimensions; ++dx) {
                #if SampleSpecificTexel_
                    uint x = SpecificTexelX_;
                    uint y = SpecificTexelY_;
                #else
                    uint x = bx * blockDimensions + dx;
                    uint y = by * blockDimensions + dy;
                #endif

                for(uint scan = 0; scan < raysPerPixel; ++scan) {
                    CreatePrimaryRay(kernelContext, y * width + x, x, y);
                }
            }
        }

        float pixelWeight = (1.0f / raysPerPixel);
        for(uint dy = 0; dy < blockDimensions; ++dy) {
            for(uint dx = 0; dx < blockDimensions; ++dx) {
                #if SampleSpecificTexel_
                    uint x = SpecificTexelX_;
                    uint y = SpecificTexelY_;
                #else
                    uint x = bx * blockDimensions + dx;
                    uint y = by * blockDimensions + dy;
                #endif

                integratorContext->imageData[y * width + x] = pixelWeight * integratorContext->imageData[y * width + x];
            }
        }
    }

    //==============================================================================
    static void PathTracerKernel(void* userData)
    {
        Random::MersenneTwister twister;
        Random::MersenneTwisterInitialize(&twister, 0);

        PathTracerContext* integratorContext = static_cast<PathTracerContext*>(userData);

        KernelContext kernelContext;
        kernelContext.sceneData = integratorContext->sceneData;
        kernelContext.camera    = &integratorContext->camera;
        kernelContext.imageData = integratorContext->imageData;
        kernelContext.twister   = &twister;
        kernelContext.rayStackCapacity = 1024 * 1024;
        kernelContext.rayStackCount = 0;
        kernelContext.rayStack = AllocArrayAligned_(Ray, kernelContext.rayStackCapacity, CacheLineSize_);

        do {
            uint64 blockIndex = (uint64)Atomic::Increment64(integratorContext->consumedBlocks) - 1;
            if(blockIndex >= integratorContext->blockCount)
                break;

            RayCastImageBlock(integratorContext, &kernelContext, (uint)blockIndex);

            Atomic::Increment64(integratorContext->completedBlocks);
        }
        while(true);

        FreeAligned_(kernelContext.rayStack);
        
        Random::MersenneTwisterShutdown(&twister);
    }

    //==============================================================================
    void PathTraceImage(SceneContext& context, uint width, uint height, float3* imageData)
    {
        const SceneResource* scene   = context.scene;
        SceneResourceData* sceneData = scene->data;

        uint blockDimensions = 16;
        AssertMsg_(blockDimensions % 16 == 0, "Naive block splitting is temp so I'm ignoring obvious edge cases for now");
        AssertMsg_(blockDimensions % 16 == 0, "Naive block splitting is temp so I'm ignoring obvious edge cases for now");

        uint blockCountX = width / blockDimensions;
        uint blockCountY = height / blockDimensions;
        uint blockCount = blockCountX * blockCountY;

        float aspect = (float)width / height;
        float verticalFov = 2.0f * Math::Atanf(sceneData->camera.fov * 0.5f) * aspect;

        float4x4 projection = PerspectiveFovLhProjection(verticalFov, aspect, sceneData->camera.znear, sceneData->camera.zfar);
        float4x4 view = LookAtLh(sceneData->camera.position, sceneData->camera.up, sceneData->camera.lookAt);
        float4x4 viewProj = MatrixMultiply(view, projection);

        RayCastCameraSettings camera;
        camera.invViewProjection = MatrixInverse(viewProj);
        camera.viewportWidth     = (float)width;
        camera.viewportHeight    = (float)height;
        camera.position          = sceneData->camera.position;
        camera.znear             = sceneData->camera.znear;
        camera.zfar              = sceneData->camera.zfar;

        int64 consumedBlocks = 0;
        int64 completedBlocks = 0;

        PathTracerContext integratorContext;
        integratorContext.sceneData         = &context;
        integratorContext.camera            = camera;
        integratorContext.blockDimensions   = blockDimensions;
        integratorContext.blockCountX       = blockCountX;
        integratorContext.blockCountY       = blockCountY;
        integratorContext.blockCount        = blockCount;
        integratorContext.imageData         = imageData;
        integratorContext.width             = width;
        integratorContext.height            = height;
        integratorContext.consumedBlocks    = &consumedBlocks;
        integratorContext.completedBlocks   = &completedBlocks;

        #if EnableMultiThreading_ 
            const uint threadCount = 7;
            ThreadHandle threadHandles[threadCount];

            // -- fork threads
            for(uint scan = 0; scan < threadCount; ++scan) {
                threadHandles[scan] = CreateThread(PathTracerKernel, &integratorContext);
            }
        #endif

        // -- do work on the main thread too
        PathTracerKernel(&integratorContext);

        #if EnableMultiThreading_ 
            // -- wait for any other threads to finish
            while(*integratorContext.completedBlocks != blockCount);

            for(uint scan = 0; scan < threadCount; ++scan) {
                ShutdownThread(threadHandles[scan]);
            }
        #endif
    }
}