
//==============================================================================
// Joe Schutte
//==============================================================================

#include "PathTracer.h"
#include "PathTracerShading.h"

#include <SceneLib/SceneResource.h>
#include <SceneLib/ImageBasedLightResource.h>
#include <TextureLib/TextureFiltering.h>
#include <TextureLib/TextureResource.h>
#include <GeometryLib/Camera.h>
#include <GeometryLib/Ray.h>
#include <GeometryLib/SurfaceDifferentials.h>
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
    inline void CoordinateSystem(const float3& v1, float3* v2, float3* v3)
    {
        if(Math::Absf(v1.x) > Math::Absf(v1.y))
            *v2 = float3(-v1.z, 0, v1.x) * (1.0f / Math::Sqrtf(v1.x * v1.x + v1.z * v1.z));
        else
            *v2 = float3(0, v1.z, -v1.y) * (1.0f / Math::Sqrtf(v1.y * v1.y + v1.z * v1.z));
        *v3 = Cross(v1, *v2);
    }

    //==============================================================================
    static void CalculateSurfaceParams(const SceneResourceData* __restrict scene, uint32 primitiveId, float2 barycentric,
                                       float3& normal, float3& dpdu, float3& dpdv, float3& dndu, float3& dndv, float2& uvs, uint16& materialIndex)
    {
        uint32 i0 = scene->indices[3 * primitiveId + 0];
        uint32 i1 = scene->indices[3 * primitiveId + 1];
        uint32 i2 = scene->indices[3 * primitiveId + 2];

        const VertexAuxiliaryData& v0 = scene->vertexData[i0];
        const VertexAuxiliaryData& v1 = scene->vertexData[i1];
        const VertexAuxiliaryData& v2 = scene->vertexData[i2];
        float3 p0wtf                     = scene->positions[i0].XYZ();
        float3 p1wtf                     = scene->positions[i1].XYZ();
        float3 p2wtf                     = scene->positions[i2].XYZ();

        float3 p0  = float3(v0.px, v0.py, v0.pz);
        float3 p1  = float3(v1.px, v1.py, v1.pz);
        float3 p2  = float3(v2.px, v2.py, v2.pz);
        float3 n0  = float3(v0.nx, v0.ny, v0.nz);
        float3 n1  = float3(v1.nx, v1.ny, v1.nz);
        float3 n2  = float3(v2.nx, v2.ny, v2.nz);
        float2 uv0 = float2(v0.u, v0.v);
        float2 uv1 = float2(v1.u, v1.v);
        float2 uv2 = float2(v2.u, v2.v);

        float b0 = Saturate(1.0f - (barycentric.x + barycentric.y));
        float b1 = barycentric.x;
        float b2 = barycentric.y;

        // Compute deltas for triangle partial derivatives
        float2 duv02 = uv0 - uv2;
        float2 duv12 = uv1 - uv2;
        float determinant = duv02.x * duv12.y - duv02.y * duv12.x;
        bool degenerateUV = Math::Absf(determinant) < SmallFloatEpsilon_;
        if(!degenerateUV) {
            float3 edge02 = p0 - p2;
            float3 edge12 = p1 - p2;
            float3 dn02 = n0 - n2;
            float3 dn12 = n1 - n2;

            float invDet = 1 / determinant;
            dpdu = ( duv12.y * edge02 - duv02.y * edge12) * invDet;
            dpdv = (-duv12.x * edge02 + duv02.x * edge12) * invDet;
            dndu = ( duv12.y * dn02 - duv02.y * dn12) * invDet;
            dndv = (-duv12.x * dn02 + duv02.x * dn12) * invDet;
        }
        if(degenerateUV || LengthSquared(Cross(dpdu, dpdv)) == 0.0f)
        {
            CoordinateSystem(Normalize(Cross(p2 - p0, p1 - p0)), &dpdu, &dpdv);
            dndu = float3::Zero_;
            dndv = float3::Zero_;
        }

        normal        = Normalize(b0 * n0  + b1 * n1  + b2 * n2);
        uvs           = b0 * uv0 + b1 * uv1 + b2 * uv2;
        materialIndex = v0.materialIndex;
    }

    //==============================================================================
    static bool RayPick(const RTCScene& rtcScene, const SceneResourceData* scene, const Ray& ray,
                        float3& position, float2& baryCoords, int32& primId)
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

        return true;
    }

    //==============================================================================
    static Material LookupMaterial()
    {
        // -- hack material
        Material material;
        material.specularColor = float3(1.0f, 1.0f, 1.0f);
        material.albedo        = float3(1.0f, 1.0f, 1.0f);
        material.roughness     = 0.01f;

        return material;
    }

    //==============================================================================
    static float3 CastIncoherentRay(PathTracerKernelContext* kernelContext, Random::MersenneTwister* twister, const Ray& ray, uint bounceCount)
    {
        const float bias = 0.00001f;

        const RayCastCameraSettings& camera = kernelContext->camera;
        RTCScene rtcScene                   = kernelContext->context->rtcScene;
        SceneResourceData* scene            = kernelContext->context->scene;
        TextureResourceData* textures       = kernelContext->context->textures;
        ImageBasedLightResourceData* ibl    = kernelContext->context->ibl;

        float3 newPosition;
        float2 baryCoords;
        int32 primId;
        bool hit = RayPick(rtcScene, scene, ray, newPosition, baryCoords, primId);

        if(hit) {
            float3 n;
            float3 dpdu, dpdv;
            float3 dndu, dndv;
            float2 uvs;
            uint16 materialIndex;
            CalculateSurfaceParams(scene, primId, baryCoords, n, dpdu, dpdv, dndu, dndv, uvs, materialIndex);

            SurfaceDifferentials differentials;
            CalculateSurfaceDifferentials(ray, n, newPosition, dpdu, dpdv, differentials);

            if(materialIndex == 0) {
                return TextureFiltering::EWA(&textures[0], uvs, differentials.duvdx, differentials.duvdy);
            }

            float3 v = -ray.direction;

            Material material = LookupMaterial();

            float3 wi;
            float3 reflectance;
            if(bounceCount == 3) {
                return float3::Zero_;
            }
            else {
                MIS(rtcScene, ibl, twister, newPosition, n, v, &material, wi, reflectance);
                if(Dot(reflectance, float3(1, 1, 1)) <= 0.0f)
                    return float3::Zero_;

                Ray bounceRay = MakeRay(newPosition + bias * n, wi, bias, FloatMax_);
                return reflectance * CastIncoherentRay(kernelContext, twister, bounceRay, bounceCount + 1);
            }
        }
        else {
            return float3::Zero_;
        }
    }

    //==============================================================================
    static float3 CastPrimaryRay(PathTracerKernelContext* kernelContext, Random::MersenneTwister* twister, uint x, uint y)
    {
        const float bias = 0.0001f;

        const RayCastCameraSettings& camera = kernelContext->camera;
        RTCScene rtcScene                   = kernelContext->context->rtcScene;
        SceneResourceData* scene            = kernelContext->context->scene;
        TextureResourceData* textures       = kernelContext->context->textures;
        ImageBasedLightResourceData* ibl    = kernelContext->context->ibl;

        Ray ray = JitteredCameraRay(&camera, twister, (float)x, (float)y);

        float3 position;
        float2 baryCoords;
        int32 primId;
        bool hit = RayPick(rtcScene, scene, ray, position, baryCoords, primId);

        if(hit) {
            float3 n;
            float3 dpdu, dpdv;
            float3 dndu, dndv;
            float2 uvs;
            uint16 materialIndex;
            CalculateSurfaceParams(scene, primId, baryCoords, n, dpdu, dpdv, dndu, dndv, uvs, materialIndex);

            SurfaceDifferentials differentials;
            CalculateSurfaceDifferentials(ray, n, position, dpdu, dpdv, differentials);

            if(materialIndex == 0) {
                return TextureFiltering::EWA(&textures[0], uvs, differentials.duvdx, differentials.duvdy);
            }

            float3 v = -ray.direction;

            Material material = LookupMaterial();

            float3 wi;
            float3 reflectance;
            MIS(rtcScene, ibl, twister, position, n, v, &material, wi, reflectance);
            if(Dot(reflectance, float3(1, 1, 1)) <= 0.0f)
                return float3::Zero_;

            Ray bounceRay;
            if(ray.hasDifferentials) {
                bounceRay = MakeDifferentialRay(ray.rxDirection, ray.ryDirection, position, n, v, wi, differentials, dndu, dndv, bias, FloatMax_);
            }
            else {
                bounceRay = MakeRay(position + bias * n, wi, bias, FloatMax_);
            }

            return reflectance * CastIncoherentRay(kernelContext, twister, bounceRay, 1);
        }

        return float3::Zero_;
    }

    //==============================================================================
    static void RayCastImageBlock(PathTracerKernelContext* kernelContext, uint blockIndex, Random::MersenneTwister* twister)
    {
        RTCScene rtcScene                = kernelContext->context->rtcScene;
        SceneResourceData* scene         = kernelContext->context->scene;
        ImageBasedLightResourceData* ibl = kernelContext->context->ibl;
        uint width                       = kernelContext->context->width;
        uint height                      = kernelContext->context->height;

        const RayCastCameraSettings& camera = kernelContext->camera;
        uint blockDimensions = kernelContext->blockDimensions;

        uint by = blockIndex / kernelContext->blockCountX;
        uint bx = blockIndex % kernelContext->blockCountX;

        const uint raysPerPixel = 720;

        for(uint dy = 0; dy < blockDimensions; ++dy) {
            for(uint dx = 0; dx < blockDimensions; ++dx) {
                uint x = bx * blockDimensions + dx;
                uint y = by * blockDimensions + dy;

                float3 color = float3::Zero_;
                for(uint scan = 0; scan < raysPerPixel; ++scan) {
                    color += CastPrimaryRay(kernelContext, twister, x, y);
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
        float verticalFov = 2.0f * Math::Atanf(scene->camera.fov * 0.5f) * aspect;

        float4x4 projection = PerspectiveFovLhProjection(verticalFov, aspect, scene->camera.znear, scene->camera.zfar);
        float4x4 view = LookAtLh(scene->camera.position, scene->camera.up, scene->camera.lookAt);
        float4x4 viewProj = MatrixMultiply(view, projection);

        RayCastCameraSettings camera;
        camera.invViewProjection = MatrixInverse(viewProj);
        camera.viewportWidth  = (float)width;
        camera.viewportHeight = (float)height;
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

        const uint threadCount = 7;
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