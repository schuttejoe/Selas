
//==============================================================================
// Joe Schutte
//==============================================================================

#include "PathTracer.h"
#include "PathTracerShading.h"
#include "SurfaceParameters.h"

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

#define EnableMultiThreading_       1
#define RaysPerPixel_               512
#define MaxBounceCount_             4

namespace Shooty
{
    struct PathTracerKernelContext
    {
        const SceneContext* context;
        RayCastCameraSettings camera;
        uint blockDimensions;
        uint blockCountX;
        uint blockCountY;
        uint blockCount;
        volatile int64* consumedBlocks;
        volatile int64* completedBlocks;

        float3* imageData;
    };

    //==============================================================================
    static bool RayPick(const RTCScene& rtcScene, const SceneResourceData* scene, const Ray& ray,
                        float3& position, float2& baryCoords, int32& primId, float& error)
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

        position.x = rayhit.ray.org_x + rayhit.ray.tfar * ray.direction.x;
        position.y = rayhit.ray.org_y + rayhit.ray.tfar * ray.direction.y;
        position.z = rayhit.ray.org_z + rayhit.ray.tfar * ray.direction.z;
        baryCoords = { rayhit.hit.u, rayhit.hit.v };
        primId = rayhit.hit.primID;

        error = 32.0f * 1.19209e-07f * Max(Max(Math::Absf(position.x), Math::Absf(position.y)), Max(Math::Absf(position.z), rayhit.ray.tfar));

        return true;
    }

    //==============================================================================
    static float3 CastIncoherentRay(PathTracerKernelContext* kernelContext, Random::MersenneTwister* twister, const Ray& ray, uint bounceCount)
    {
        const RayCastCameraSettings& camera = kernelContext->camera;
        RTCScene rtcScene                   = kernelContext->context->rtcScene;
        SceneResource* scene                = kernelContext->context->scene;
        ImageBasedLightResourceData* ibl    = kernelContext->context->ibl;

        float3 newPosition;
        float2 baryCoords;
        int32 primId;
        float error;
        bool hit = RayPick(rtcScene, scene->data, ray, newPosition, baryCoords, primId, error);

        float3 Lo = float3::Zero_;

        if(hit) {

            SurfaceParameters surface;
            CalculateSurfaceParams(scene, ray, newPosition, primId, baryCoords, surface);

            Lo += surface.emissive;
            Lo += CalculateDirectLighting(scene, twister, surface);

            if(bounceCount < MaxBounceCount_ && surface.materialFlags & eHasReflectance) {
                
                float3 v = -ray.direction;
                float3 wo = Normalize(MatrixMultiplyFloat3(v, surface.worldToTangent));

                float3 wi;
                float3 reflectance;
                ImportanceSampleDisneyBrdf(twister, surface, wo, wi, reflectance);
                if(Dot(reflectance, float3(1, 1, 1)) > 0.0f) {

                    wi = MatrixMultiplyFloat3(wi, surface.tangentToWorld);

                    float offsetDirection = Dot(wi, surface.normal) < 0.0f ? -1.0f : 1.0f;
                    float3 newOrigin = newPosition + offsetDirection * error * surface.normal;

                    Ray bounceRay;
                    if((surface.materialFlags & ePreserveRayDifferentials) && ray.hasDifferentials) {
                        bounceRay = MakeDifferentialRay(ray.rxDirection, ray.ryDirection, newOrigin, surface.normal, wo, wi, surface.differentials, error, FloatMax_);
                    }
                    else {
                        bounceRay = MakeRay(newOrigin, wi, error, FloatMax_);
                    }

                    Lo += reflectance * CastIncoherentRay(kernelContext, twister, bounceRay, bounceCount + 1);
                }
            }
        }

        return Lo;
    }

    //==============================================================================
    static float3 CastPrimaryRay(PathTracerKernelContext* kernelContext, Random::MersenneTwister* twister, uint x, uint y)
    {
        const RayCastCameraSettings& camera = kernelContext->camera;
        Ray ray = JitteredCameraRay(&camera, twister, (float)x, (float)y);
        return CastIncoherentRay(kernelContext, twister, ray, 0);
    }

    //==============================================================================
    static void RayCastImageBlock(PathTracerKernelContext* kernelContext, uint blockIndex, Random::MersenneTwister* twister)
    {
        RTCScene rtcScene                = kernelContext->context->rtcScene;
        ImageBasedLightResourceData* ibl = kernelContext->context->ibl;
        uint width                       = kernelContext->context->width;
        uint height                      = kernelContext->context->height;

        const RayCastCameraSettings& camera = kernelContext->camera;
        uint blockDimensions = kernelContext->blockDimensions;

        uint by = blockIndex / kernelContext->blockCountX;
        uint bx = blockIndex % kernelContext->blockCountX;

        const uint raysPerPixel = RaysPerPixel_;

        for(uint dy = 0; dy < blockDimensions; ++dy) {
            for(uint dx = 0; dx < blockDimensions; ++dx) {
                uint x = bx * blockDimensions + dx;
                uint y = by * blockDimensions + dy;

                float3 color = float3::Zero_;
                for(uint scan = 0; scan < raysPerPixel; ++scan) {
                    float3 sample = CastPrimaryRay(kernelContext, twister, x, y);
                    color += sample;
                }
                color = (1.0f / raysPerPixel) * color;
                kernelContext->imageData[y * width + x] = color;
            }
        }
    }

    //==============================================================================
    static void PathTracerKernel(void* userData)
    {
        PathTracerKernelContext* kernelContext = static_cast<PathTracerKernelContext*>(userData);

        Random::MersenneTwister twister;
        Random::MersenneTwisterInitialize(&twister, 0);

        do {
            uint64 blockIndex = (uint64)Atomic::Increment64(kernelContext->consumedBlocks) - 1;
            if(blockIndex >= kernelContext->blockCount)
                break;

            RayCastImageBlock(kernelContext, (uint)blockIndex, &twister);

            Atomic::Increment64(kernelContext->completedBlocks);
        }
        while(true);

        Random::MersenneTwisterShutdown(&twister);
    }

    //==============================================================================
    void PathTraceImage(const SceneContext& context, float3* imageData)
    {
        SceneResource* scene         = context.scene;
        SceneResourceData* sceneData = scene->data;
        uint width                   = context.width;
        uint height                  = context.height;

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
        camera.viewportWidth  = (float)width;
        camera.viewportHeight = (float)height;
        camera.position = sceneData->camera.position;
        camera.znear = sceneData->camera.znear;
        camera.zfar = sceneData->camera.zfar;

        int64 consumedBlocks = 0;
        int64 completedBlocks = 0;

        PathTracerKernelContext kernelContext;
        kernelContext.context         = &context;
        kernelContext.camera          = camera;
        kernelContext.blockDimensions = blockDimensions;
        kernelContext.blockCountX     = blockCountX;
        kernelContext.blockCountY     = blockCountY;
        kernelContext.blockCount      = blockCount;
        kernelContext.imageData       = imageData;
        kernelContext.consumedBlocks  = &consumedBlocks;
        kernelContext.completedBlocks = &completedBlocks;

        #if EnableMultiThreading_ 
            const uint threadCount = 7;
            ThreadHandle threadHandles[threadCount];

            // -- fork threads
            for(uint scan = 0; scan < threadCount; ++scan) {
                threadHandles[scan] = CreateThread(PathTracerKernel, &kernelContext);
            }
        #endif

        // -- do work on the main thread too
        PathTracerKernel(&kernelContext);

        #if EnableMultiThreading_ 
            // -- wait for any other threads to finish
            while(*kernelContext.completedBlocks != blockCount);

            for(uint scan = 0; scan < threadCount; ++scan) {
                ShutdownThread(threadHandles[scan]);
            }
        #endif
    }
}