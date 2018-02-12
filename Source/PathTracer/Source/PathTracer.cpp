
//==============================================================================
// Joe Schutte
//==============================================================================

#include "PathTracer.h"
#include "PathTracerShading.h"

#include <SceneLib/SceneResource.h>
#include <SceneLib/ImageBasedLightResource.h>
#include <UtilityLib/Color.h>
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
#include <SystemLib/BasicTypes.h>

#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>


namespace Shooty
{
    //==============================================================================
    struct RayCastCameraSettings
    {
        float4x4 invViewProjection;
        Rect     viewport;
        float3   position;
        float    znear;
        float    zfar;
    };

    //==============================================================================
    static void CalculateNormalAndUvs(const SceneResourceData* __restrict scene, uint32 primitiveId, float2 barycentric, float3& normal, float2& uvs)
    {
        uint32 i0 = scene->indices[3 * primitiveId + 0];
        uint32 i1 = scene->indices[3 * primitiveId + 1];
        uint32 i2 = scene->indices[3 * primitiveId + 2];

        float3 n0 = scene->normals[i0];
        float3 n1 = scene->normals[i1];
        float3 n2 = scene->normals[i2];

        float2 t0 = float2::Zero_;//scene->uv0[i0];
        float2 t1 = float2::Zero_;//scene->uv0[i1];
        float2 t2 = float2::Zero_;//scene->uv0[i2];

        float b0 = Saturate(1.0f - (barycentric.x + barycentric.y));
        float b1 = barycentric.x;
        float b2 = barycentric.y;

        normal = b0 * n0 + b1 * n1 + b2 * n2;
        uvs = b0 * t0 + b1 * t1 + b2 * t2;
    }

    //==============================================================================
    static float3 MakeCameraRayDirection(const RayCastCameraSettings* __restrict camera, uint viewX, uint viewY, uint width, uint height)
    {
        float x = (2.0f * viewX) / width - 1.0f;
        float y = 1.0f - (2.0f * viewY) / height;

        float4 un = MatrixMultiplyFloat4(float4(x, y, 0.0f, 1.0f), camera->invViewProjection);
        float3 worldPosition = (1.0f / un.w) * float3(un.x, un.y, un.z);

        return Normalize(worldPosition - camera->position);
    }

    //==============================================================================
    static bool RayPick(const RTCScene& rtcScene, const SceneResourceData* scene, float3 orig, float3 direction, float znear, float zfar,
                        float3& position, float2& baryCoords, int32& primId)
    {

        RTCIntersectContext context;
        rtcInitIntersectContext(&context);

        __declspec(align(16)) RTCRayHit rayhit;
        rayhit.ray.org_x = orig.x;
        rayhit.ray.org_y = orig.y;
        rayhit.ray.org_z = orig.z;
        rayhit.ray.dir_x = direction.x;
        rayhit.ray.dir_y = direction.y;
        rayhit.ray.dir_z = direction.z;
        rayhit.ray.tnear = znear;
        rayhit.ray.tfar = zfar;

        rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        rayhit.hit.primID = RTC_INVALID_GEOMETRY_ID;

        rtcIntersect1(rtcScene, &context, &rayhit);

        position.x = rayhit.ray.org_x + rayhit.ray.tfar * direction.x;
        position.y = rayhit.ray.org_y + rayhit.ray.tfar * direction.y;
        position.z = rayhit.ray.org_z + rayhit.ray.tfar * direction.z;
        baryCoords = { rayhit.hit.u, rayhit.hit.v };
        primId = rayhit.hit.primID;

        return rayhit.hit.geomID != -1;
    }

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

        uint32* imageData;
    };

    //==============================================================================
    static void RayCastImageBlock(PathTracerKernelContext* kernelContext, uint blockIndex, Random::MersenneTwister* twister)
    {
        RTCScene rtcScene                = kernelContext->context->rtcScene;
        SceneResourceData* scene         = kernelContext->context->scene;
        ImageBasedLightResourceData* ibl = kernelContext->context->ibl;
        uint width                       = kernelContext->context->width;
        uint height                      = kernelContext->context->height;

        const RayCastCameraSettings& camera = kernelContext->camera;
        uint32* imageData = kernelContext->imageData;
        uint blockDimensions = kernelContext->blockDimensions;

        const float3 lightIntensity = 0.0f * float3(226.0f / 255.0f, 197.0f / 255.0f, 168.0f / 255.0f);
        const float3 lightPosition = float3(15.0f, 30.0f, 45.0f);
        const float roughness = 0.7f;
        const float3 reflectance = float3(0.1f, 0.1f, 0.1f);
        const float3 albedo = float3(0.8f, 0.8f, 0.8f);

        uint by = blockIndex / kernelContext->blockCountX;
        uint bx = blockIndex % kernelContext->blockCountX;

        for(uint dy = 0; dy < blockDimensions; ++dy) {
            for(uint dx = 0; dx < blockDimensions; ++dx) {
                uint x = bx * blockDimensions + dx;
                uint y = by * blockDimensions + dy;

                float3 direction = MakeCameraRayDirection(&camera, x, y, width, height);

                float3 position;
                float2 baryCoords;
                int32 primId;
                bool hit = RayPick(rtcScene, scene, scene->camera.position, direction, scene->camera.znear, scene->camera.zfar, position, baryCoords, primId);

                float3 color = float3::Zero_;

                if(hit) {
                    float3 n;
                    float2 uvs;
                    CalculateNormalAndUvs(scene, primId, baryCoords, n, uvs);

                    float3 v = Normalize(scene->camera.position - position);

                    // -- kinda hacked ibl
                    float3 iblContrib = ImportanceSampleIbl(rtcScene, ibl, twister, position, n, v, albedo, reflectance, roughness);

                    color = iblContrib;
                }

                imageData[y * width + x] = ColorRGBA(float4(color, 1.0f));
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
    void GenerateRayCastImage(const SceneContext& context, uint32* imageData)
    {
        SceneResourceData* scene         = context.scene;
        uint width                       = context.width;
        uint height                      = context.height;

        uint blockDimensions = 16;
        AssertMsg_(blockDimensions % 16 == 0, "Naive block splitting is temp so I'm ignoring obvious edge cases for now");
        AssertMsg_(blockDimensions % 16 == 0, "Naive block splitting is temp so I'm ignoring obvious edge cases for now");

        uint blockCountX = width / blockDimensions;
        uint blockCountY = height / blockDimensions;
        uint blockCount = blockCountX * blockCountY;

        float aspect = (float)width / height;

        float4x4 projection = PerspectiveFovLhProjection(scene->camera.fov, aspect, scene->camera.znear, scene->camera.zfar);
        float4x4 view = LookAtLh(scene->camera.position, float3::YAxis_, scene->camera.lookAt);
        float4x4 viewProj = MatrixMultiply(view, projection);

        RayCastCameraSettings camera;
        camera.invViewProjection = MatrixInverse(viewProj);
        camera.viewport = { 0, 0, (sint)width, (sint)height };
        camera.position = scene->camera.position;
        camera.znear = scene->camera.znear;
        camera.zfar = scene->camera.zfar;

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

        const uint threadCount = 8;
        ThreadHandle threadHandles[threadCount];

        // -- fork threads
        for(uint scan = 0; scan < threadCount; ++scan) {
            threadHandles[scan] = CreateThread(PathTracerKernel, &kernelContext);
        }
        
        // -- do work on the main thread too
        PathTracerKernel(&kernelContext);

        // -- wait for any other threads to finish
        while(*kernelContext.completedBlocks != blockCount);

        for(uint scan = 0; scan < threadCount; ++scan) {
            ShutdownThread(threadHandles[scan]);
        }
    }
}