
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

#define SampleSpecificTexel_    0
#define SpecificTexelX_         400
#define SpecificTexelY_         550

#if SampleSpecificTexel_
#define EnableMultiThreading_   0
#define RaysPerPixel_           1
#else
#define EnableMultiThreading_   1
#define RaysPerPixel_           128
#endif
#define MaxBounceCount_         10

namespace Shooty
{
    //==============================================================================
    struct PathTracerContext
    {
        const SceneContext* sceneData;
        RayCastCameraSettings camera;
        uint blockDimensions;
        uint blockCountX;
        uint blockCountY;
        uint blockCount;
        uint width;
        uint height;
        volatile int64* consumedBlocks;
        volatile int64* completedBlocks;
        volatile int64* kernelSeedAtomic;

        float3* imageData;
    };



    //==============================================================================
    static bool RayPick(const RTCScene& rtcScene, const SceneResourceData* scene, const Ray& ray,
                        HitParameters& hit)
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
        rayhit.ray.tnear = ray.tnear;
        rayhit.ray.tfar  = ray.tfar;

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

        hit.rxOrigin      = ray.rxOrigin;
        hit.rxDirection   = ray.rxDirection;
        hit.ryOrigin      = ray.ryOrigin;
        hit.ryDirection   = ray.ryDirection;
        hit.viewDirection = -ray.direction;
        hit.ior           = ray.mediumIOR;

        return true;
    }

    //==============================================================================
    static Ray CreateBounceRay(const SurfaceParameters& surface, float3 wo, float3 wi)
    {
        float3 offsetOrigin = OffsetRayOrigin(surface, wi, 1.0f);

        bool rayHasDifferentials = surface.rxDirection.x != 0 || surface.rxDirection.y != 0;

        Ray bounceRay;
        if((surface.materialFlags & ePreserveRayDifferentials) && rayHasDifferentials) {
            bounceRay = MakeDifferentialRay(surface.rxDirection, surface.ryDirection, offsetOrigin, surface.geometricNormal, wo, wi, surface.differentials, surface.error, FloatMax_, surface.ior);
        }
        else {
            bounceRay = MakeRay(offsetOrigin, wi, surface.error, FloatMax_, surface.ior);
        }

        return bounceRay;
    }

    //==============================================================================
    static float3 CastSingleRay(KernelContext* context, const Ray& ray, uint bounceCount)
    {
        if(bounceCount == MaxBounceCount_) {
            return float3::Zero_;
        }

        RTCScene rtcScene                   = context->sceneData->rtcScene;
        SceneResource* scene                = context->sceneData->scene;
        ImageBasedLightResourceData* ibl    = context->sceneData->ibl;

        HitParameters hit;
        bool rayCastHit = RayPick(rtcScene, scene->data, ray, hit);

        float3 Lo = float3::Zero_;

        if(rayCastHit) {

            SurfaceParameters surface;
            if(CalculateSurfaceParams(context, &hit, surface) == false) {
                return float3::Zero_;
            }

            float3 v = hit.viewDirection;
            float3 wo = Normalize(MatrixMultiply(v, surface.worldToTangent));

            Lo += surface.emissive;
            Lo += CalculateDirectLighting(rtcScene, scene, context->twister, surface, v);

            float3 wi;
            float3 reflectance;
            float ior = hit.ior;

            if(surface.shader == eDisney) {
                ImportanceSampleIbl(rtcScene, ibl, context->twister, surface, v, wo, ior, wi, reflectance, ior);
                //ImportanceSampleDisneyBrdf(twister, surface, wo, ray.mediumIOR, wi, reflectance, ior);
            }
            else if(surface.shader == eTransparentGgx) {
                ImportanceSampleTransparent(context->twister, surface, wo, hit.ior, wi, reflectance, ior);
            }
            else {
                return float3(100.0f, 0.0f, 0.0f);
            }

            wi = Normalize(MatrixMultiply(wi, surface.tangentToWorld));

            Ray bounceRay = CreateBounceRay(surface, v, wi);

            Lo += reflectance * CastSingleRay(context, bounceRay, bounceCount + 1);
        }
        else {
            Lo += SampleIbl(ibl, ray.direction);
        }

        return Lo;
    }

    //==============================================================================
    static float3 CastPrimaryRay(KernelContext* context, uint x, uint y)
    {
        Ray ray = JitteredCameraRay(context->camera, context->twister, (float)x, (float)y);
        return CastSingleRay(context, ray, 0);
    }

    //==============================================================================
    static void RayCastImageBlock(const PathTracerContext* integratorContext, KernelContext* kernelContext, uint blockIndex, Random::MersenneTwister* twister)
    {
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

                float3 color = float3::Zero_;
                for(uint scan = 0; scan < raysPerPixel; ++scan) {
                    float3 sample = CastPrimaryRay(kernelContext, x, y);
                    color += sample;
                }
                color = (1.0f / raysPerPixel) * color;
                integratorContext->imageData[y * width + x] = color;
            }
        }
    }

    //==============================================================================
    static void PathTracerKernel(void* userData)
    {
        PathTracerContext* integratorContext = static_cast<PathTracerContext*>(userData);

        int64 seed = Atomic::Increment64(integratorContext->kernelSeedAtomic);

        Random::MersenneTwister twister;
        Random::MersenneTwisterInitialize(&twister, (uint32)seed);

        KernelContext kernelContext;
        kernelContext.sceneData = integratorContext->sceneData;
        kernelContext.camera    = &integratorContext->camera;
        kernelContext.twister   = &twister;

        do {
            uint64 blockIndex = (uint64)Atomic::Increment64(integratorContext->consumedBlocks) - 1;
            if(blockIndex >= integratorContext->blockCount)
                break;

            RayCastImageBlock(integratorContext, &kernelContext, (uint)blockIndex, &twister);

            Atomic::Increment64(integratorContext->completedBlocks);
        }
        while(true);

        Random::MersenneTwisterShutdown(&twister);
    }

    //==============================================================================
    void PathTraceImage(const SceneContext& context, uint width, uint height, float3* imageData)
    {
        SceneResource* scene         = context.scene;
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
        int64 kernelSeedAtomic = 0;

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
        integratorContext.kernelSeedAtomic  = &kernelSeedAtomic;

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