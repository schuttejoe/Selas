
//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "PathTracer.h"
#include "SceneLib/SceneResource.h"
#include "SceneLib/GeometryCache.h"
#include "Shading/SurfaceScattering.h"
#include "Shading/VolumetricScattering.h"
#include "Shading/SurfaceParameters.h"
#include "Shading/IntegratorContexts.h"
#include "Shading/AreaLighting.h"
#include "Shading/PathTracingBatcher.h"
#include "GeometryLib/Camera.h"
#include "GeometryLib/Ray.h"
#include "MathLib/FloatFuncs.h"
#include "MathLib/FloatStructs.h"
#include "MathLib/Trigonometric.h"
#include "MathLib/ImportanceSampling.h"
#include "MathLib/Random.h"
#include "ThreadingLib/Thread.h"
#include "SystemLib/OSThreading.h"
#include "SystemLib/Atomic.h"
#include "SystemLib/MemoryAllocation.h"
#include "SystemLib/Memory.h"
#include "SystemLib/BasicTypes.h"
#include "SystemLib/MinMax.h"
#include "SystemLib/SystemTime.h"
#include "SystemLib/Logging.h"
#include "SystemLib/CountOf.h"

#include "embree3/rtcore.h"
#include "embree3/rtcore_ray.h"

#define RayBatchSize_         4 Mb_
#define HitBatchSize_         2 Mb_

#define WorkerThreadCount_    9
#define SamplesPerPixelX_     4
#define SamplesPerPixelY_     4
#define OutputLayers_         1

namespace Selas
{
    namespace DeferredPathTracer
    {
        struct KernelData
        {
            const RayCastCameraSettings* camera;
            PathTracingBatcher*          ptBatcher;
            Framebuffer*                 frame;
            volatile int64               kernelCounter;
            volatile int64               pixelIndex;
            const SceneResource*         scene;
            GeometryCache*               geometryCache;
            TextureCache*                textureCache;
        };

        //=========================================================================================================================
        static void ShadeHitPosition(GIIntegratorContext* __restrict context, PathTracingBatcher* ptBatcher,
                                     const HitParameters& hit, const SurfaceParameters& surface)
        {


            // -- choose a light and sample the light source
            LightDirectSample lightSample;
            NextEventEstimation(context, surface.lightSetIndex, hit.position, GeometricNormal(surface), lightSample);
            if(Dot(lightSample.radiance, float3::One_) > 0) {
                float forwardPdfW;
                float reversePdfW;
                float3 reflectance = EvaluateBsdf(surface, hit.view, lightSample.direction, forwardPdfW,
                                                  reversePdfW);

                float weight = 1.0f;// ImportanceSampling::BalanceHeuristic(1, lightSample.pdfW, 1, forwardPdfW);

                float3 sample = weight * reflectance * lightSample.radiance * (1.0f / lightSample.pdfW);
                if(Dot(sample, float3::One_) > 0) {
                    float3 offset = OffsetRayOrigin(surface, lightSample.direction, 0.1f);

                    OcclusionRay occlusionRay;
                    occlusionRay.ray = MakeRay(offset, lightSample.direction);
                    occlusionRay.distance = lightSample.distance;
                    occlusionRay.index = hit.index;
                    occlusionRay.value = sample * hit.throughput;
                    ptBatcher->AddUnsortedOcclusionRay(occlusionRay);
                }
            }

            LightDirectSample skySample;
            SampleBackground(context, skySample);
            if(Dot(skySample.radiance, float3::One_) > 0) {
                float forwardPdfW;
                float reversePdfW;
                float3 reflectance = EvaluateBsdf(surface, hit.view, skySample.direction, forwardPdfW, reversePdfW);

                float misWeight = ImportanceSampling::BalanceHeuristic(1, skySample.pdfW, 1, forwardPdfW);

                float3 sample = misWeight * reflectance * skySample.radiance * (1.0f / skySample.pdfW);
                if(Dot(sample, float3::One_) > 0) {
                    float3 offset = OffsetRayOrigin(surface, skySample.direction, 0.1f);

                    OcclusionRay occlusionRay;
                    occlusionRay.ray = MakeRay(offset, skySample.direction);
                    occlusionRay.distance = skySample.distance;
                    occlusionRay.index = hit.index;
                    occlusionRay.value = sample * hit.throughput;
                    ptBatcher->AddUnsortedOcclusionRay(occlusionRay);
                }
            }

            {
                // - sample the bsdf
                BsdfSample bsdfSample;
                if(SampleBsdfFunction(&context->sampler, surface, hit.view, bsdfSample) == false) {
                    return;
                }

                float skyPdfW = BackgroundLightingPdf(context, bsdfSample.wi);
                float misWeight = ImportanceSampling::BalanceHeuristic(1, bsdfSample.forwardPdfW, 1, skyPdfW);

                float3 throughput = misWeight * hit.throughput * bsdfSample.reflectance;
                if(LengthSquared(throughput) == 0.0f) {
                    return;
                }

                // --Russian roulette path termination
                if(hit.trackedBounces >= MaxTrackedBounces_) {
                    float continuationProb = Max<float>(Max<float>(throughput.x, throughput.y), throughput.z);
                    if(context->sampler.UniformFloat() >= continuationProb) {
                        return;
                    }
                    Assert_(continuationProb > 0.0f);
                    throughput = throughput * (1.0f / continuationProb);
                }

                float3 offsetOrigin = OffsetRayOrigin(surface, bsdfSample.wi, 1.0f);

                DeferredRay bounceRay;
                bounceRay.error = hit.error;
                bounceRay.index = hit.index;
                bounceRay.diracScatterOnly = hit.diracScatterOnly && bsdfSample.flags & SurfaceEventFlags::eDiracEvent;
                bounceRay.ray = MakeRay(offsetOrigin, bsdfSample.wi);
                bounceRay.throughput = throughput;
                bounceRay.trackedBounces = Min<uint32>(MaxTrackedBounces_, hit.trackedBounces + 1);
                ptBatcher->AddUnsortedDeferredRay(bounceRay);
            }
        }

        //=========================================================================================================================
        static void TraceRayBatch(GIIntegratorContext* __restrict context, PathTracingBatcher* ptBatcher,
                                  DeferredRay* rays, uint rayCount)
        {
            #define BatchSize_ 8
            uint batchCount = (rayCount + BatchSize_ - 1) / BatchSize_;

            const float kErr = 32.0f * 1.19209e-07f;

            for(uint batchScan = 0; batchScan < batchCount; ++batchScan) {
                DeferredRay* startRay = rays + BatchSize_ * batchScan;

                uint batchSize = Min<uint>(rayCount - batchScan * BatchSize_, BatchSize_);

                RTCIntersectContext rtcContext;
                rtcInitIntersectContext(&rtcContext);
                rtcContext.flags = RTC_INTERSECT_CONTEXT_FLAG_COHERENT;

                Align_(64) int32 valid[BatchSize_];

                Align_(64) RTCRayHit8 rayhit;
                for(uint scan = 0; scan < batchSize; ++scan) {
                    rayhit.ray.org_x[scan] = startRay[scan].ray.origin.x;
                    rayhit.ray.org_y[scan] = startRay[scan].ray.origin.y;
                    rayhit.ray.org_z[scan] = startRay[scan].ray.origin.z;
                    rayhit.ray.dir_x[scan] = startRay[scan].ray.direction.x;
                    rayhit.ray.dir_y[scan] = startRay[scan].ray.direction.y;
                    rayhit.ray.dir_z[scan] = startRay[scan].ray.direction.z;
                    rayhit.ray.tnear[scan] = 0.0f;
                    rayhit.ray.tfar[scan] = FloatMax_;

                    rayhit.hit.geomID[scan] = RTC_INVALID_GEOMETRY_ID;
                    rayhit.hit.primID[scan] = RTC_INVALID_GEOMETRY_ID;
                    rayhit.hit.instID[0][scan] = RTC_INVALID_GEOMETRY_ID;
                    rayhit.hit.instID[1][scan] = RTC_INVALID_GEOMETRY_ID;
                    valid[scan] = -1;
                }
                for(uint scan = batchSize; scan < BatchSize_; ++scan) {
                    valid[scan] = 0;
                }

                rtcIntersect8(valid, context->rtcScene, &rtcContext, &rayhit);

                for(uint scan = 0; scan < BatchSize_; ++scan) {
                    float3 Ld[OutputLayers_];
                    Memory::Zero(Ld, sizeof(Ld));

                    if(valid[scan] == 0 || rayhit.hit.geomID[scan] == RTC_INVALID_GEOMETRY_ID) {

                        float3 sample;
                        if(startRay[scan].diracScatterOnly)
                            sample = EvaluateBackgroundMiss(context, startRay[scan].ray.direction);
                        else
                            sample = EvaluateBackground(context, startRay[scan].ray.direction);

                        Ld[0] += sample * startRay[scan].throughput;
                        FramebufferWriter_Write(&context->frameWriter, Ld, OutputLayers_, startRay[scan].index);
                        continue;
                    }

                    HitParameters hit;
                    hit.position.x       = rayhit.ray.org_x[scan] + rayhit.ray.tfar[scan] * rayhit.ray.dir_x[scan];
                    hit.position.y       = rayhit.ray.org_y[scan] + rayhit.ray.tfar[scan] * rayhit.ray.dir_y[scan];
                    hit.position.z       = rayhit.ray.org_z[scan] + rayhit.ray.tfar[scan] * rayhit.ray.dir_z[scan];
                    hit.normal           = float3(rayhit.hit.Ng_x[scan], rayhit.hit.Ng_y[scan], rayhit.hit.Ng_z[scan]);
                    hit.view             = -startRay[scan].ray.direction;
                    hit.error            = kErr * Max(Max(Math::Absf(hit.position.x), Math::Absf(hit.position.y)),
                                                      Max(Math::Absf(hit.position.z), rayhit.ray.tfar[scan]));
                    hit.baryCoords       = { rayhit.hit.u[scan], rayhit.hit.v[scan] };
                    hit.geomId           = rayhit.hit.geomID[scan];
                    hit.primId           = rayhit.hit.primID[scan];
                    hit.instId[0]        = rayhit.hit.instID[0][scan];
                    hit.instId[1]        = rayhit.hit.instID[1][scan];
                    hit.index            = startRay[scan].index;
                    hit.diracScatterOnly = startRay[scan].diracScatterOnly;
                    hit.trackedBounces   = startRay[scan].trackedBounces;
                    hit.throughput       = startRay[scan].throughput;

                    ptBatcher->AddUnsortedHit(hit);
                }
            }
        }

        //=========================================================================================================================
        static void TraceOcclusionBatch(GIIntegratorContext* __restrict context, OcclusionRay* rays, uint rayCount)
        {
            #define BatchSize_ 8
            uint batchCount = (rayCount + BatchSize_ - 1) / BatchSize_;

            for(uint batchScan = 0; batchScan < batchCount; ++batchScan) {
                OcclusionRay* startRay = rays + BatchSize_ * batchScan;

                uint batchSize = Min<uint>(rayCount - batchScan * BatchSize_, BatchSize_);

                RTCIntersectContext rtcContext;
                rtcInitIntersectContext(&rtcContext);
                rtcContext.flags = RTC_INTERSECT_CONTEXT_FLAG_COHERENT;

                Align_(64) int32 valid[BatchSize_];

                Align_(64) RTCRay8 ray;
                for(uint scan = 0; scan < batchSize; ++scan) {
                    ray.org_x[scan] = startRay[scan].ray.origin.x;
                    ray.org_y[scan] = startRay[scan].ray.origin.y;
                    ray.org_z[scan] = startRay[scan].ray.origin.z;
                    ray.dir_x[scan] = startRay[scan].ray.direction.x;
                    ray.dir_y[scan] = startRay[scan].ray.direction.y;
                    ray.dir_z[scan] = startRay[scan].ray.direction.z;
                    ray.tnear[scan] = 0.0f;
                    ray.tfar[scan]  = startRay[scan].distance;

                    valid[scan] = -1;
                }
                for(uint scan = batchSize; scan < BatchSize_; ++scan) {
                    valid[scan] = 0;
                }

                rtcOccluded8(valid, context->rtcScene, &rtcContext, &ray);

                for(uint scan = 0; scan < BatchSize_; ++scan) {
                    if(valid[scan] == -1 && ray.tfar[scan] >= 0.0f) {

                        float3 Ld[OutputLayers_];
                        Memory::Zero(Ld, sizeof(Ld));

                        Ld[0] = startRay[scan].value;
                        FramebufferWriter_Write(&context->frameWriter, Ld, OutputLayers_, startRay[scan].index);
                    }
                }
            }
        }

        //=========================================================================================================================
        static void ShadeHitBatch(GIIntegratorContext* __restrict context, PathTracingBatcher* ptBatcher,
                                  HitParameters* hits, uint hitCount)
        {
            float4x4 localToWorld;

            ModelGeometryUserData* modelData = nullptr;
            TextureHandle textureHandle;
            PtexTexture* texture = nullptr;
            Ptex::PtexFilter* filter = nullptr;

            int32 geomId = RTC_INVALID_GEOMETRY_ID;
            int32 instId[MaxInstanceLevelCount_];
            for(uint scan = 0; scan < MaxInstanceLevelCount_; ++scan) {
                instId[scan] = RTC_INVALID_GEOMETRY_ID;
            }

            Ptex::PtexFilter::Options opts(Ptex::PtexFilter::FilterType::f_bspline);

            for(uint scan = 0; scan < hitCount; ++scan) {

                const HitParameters& hit = hits[scan];

                bool modelDataChanged = (geomId != hit.geomId);
                for(uint scan = 0; !modelDataChanged && scan < MaxInstanceLevelCount_; ++scan) {
                    modelDataChanged = (instId[scan] != hit.instId[scan]);
                }

                if(modelDataChanged) {
                    ModelDataFromRayIds(context->scene, hit.instId, hit.geomId, localToWorld, modelData);
                    geomId = hit.geomId;
                    Memory::Copy(instId, hit.instId, sizeof(hit.instId));
                }

                if(modelData->baseColorTextureHandle != textureHandle) {
                    if(texture != nullptr) {
                        filter->release();
                        texture->release();
                    }

                    if(modelData->baseColorTextureHandle.Valid()) {
                        texture = context->textureCache->FetchPtex(modelData->baseColorTextureHandle);
                        filter = Ptex::PtexFilter::getFilter(texture, opts);
                    }
                    else {
                        texture = nullptr;
                        filter = nullptr;
                    }

                    textureHandle = modelData->baseColorTextureHandle;
                }

                SurfaceParameters surface;
                if(CalculateSurfaceParams(context, &hit, modelData, localToWorld, filter, surface) == false) {
                    continue;
                }

                ShadeHitPosition(context, ptBatcher, hit, surface);
            }

            if(texture != nullptr) {
                filter->release();
                texture->release();
            }
        }

        //=========================================================================================================================
        static void GeneratePrimaryRays(CSampler* sampler, KernelData* __restrict kernelData)
        {
            uint width = kernelData->camera->width;
            uint height = kernelData->camera->height;

            int64 endIndex = width * height;

            while(true) {
                int64 index = Atomic::Increment64(&kernelData->pixelIndex);
                if(index >= endIndex) {
                    break;
                }

                uint y = index / width;
                uint x = index - (y * width);

                uint sampleCount = SamplesPerPixelX_ * SamplesPerPixelY_;

                for(uint scan = 0; scan < sampleCount; ++scan) {

                    DeferredRay dr;
                    dr.ray              = JitteredCameraRay(kernelData->camera, (int32)x, (int32)y, (int32)scan,
                                                            SamplesPerPixelX_, SamplesPerPixelY_, (int32)index);
                    dr.error            = 0.0f;
                    dr.index            = (uint32)(y * width + x);
                    dr.diracScatterOnly = 1;
                    dr.throughput       = float3::One_;
                    dr.trackedBounces   = 0;
                    kernelData->ptBatcher->AddUnsortedDeferredRay(dr);
                }
            }
        }

        //=========================================================================================================================
        static void DeferredPathTracerKernel(void* userData)
        {
            KernelData* __restrict kernelData = (KernelData*)userData;
            
            int64 kernelIndex = Atomic::Increment64(&kernelData->kernelCounter);

            GIIntegratorContext context;
            context.geometryCache = kernelData->geometryCache;
            context.textureCache  = kernelData->textureCache;
            context.rtcScene      = kernelData->scene->rtcScene;
            context.scene         = kernelData->scene;
            context.camera        = kernelData->camera;
            context.sampler.Initialize((uint32)kernelIndex);
            context.maxPathLength = 1;
            FramebufferWriter_Initialize(&context.frameWriter, kernelData->frame);

            GeneratePrimaryRays(&context.sampler, kernelData);

            // JSTODO -- Change stop condition to be that this is empty and that all worker kernels report as idle
            //        -- so no threads exit when they could be useful later.
            while(kernelData->ptBatcher->Empty() == false) {
                
                DeferredRay* deferredRays;
                OcclusionRay* occlusionRays;
                HitParameters* hitParams;
                uint rayCount;
                uint hitCount;

                if(kernelData->ptBatcher->GetSortedHits(hitParams, hitCount)) {
                    ShadeHitBatch(&context, kernelData->ptBatcher, hitParams, hitCount);
                    kernelData->ptBatcher->FreeHits(hitParams);
                }
                else if(kernelData->ptBatcher->GetSortedBatch(occlusionRays, rayCount)) {
                    TraceOcclusionBatch(&context, occlusionRays, rayCount);
                    kernelData->ptBatcher->FreeRays(occlusionRays);
                }
                else if(kernelData->ptBatcher->GetSortedBatch(deferredRays, rayCount)) {
                    TraceRayBatch(&context, kernelData->ptBatcher, deferredRays, rayCount);
                    kernelData->ptBatcher->FreeRays(deferredRays);
                }
                else {
                    kernelData->ptBatcher->Flush();
                }
            }

            context.sampler.Shutdown();
            FramebufferWriter_Shutdown(&context.frameWriter);
        }

        //=========================================================================================================================
        void GenerateImage(GeometryCache* geometryCache, TextureCache* textureCache, SceneResource* scene,
                           const RayCastCameraSettings& camera, cpointer imageName)
        {
            PathTracingBatcher ptBatcher;
            ptBatcher.Initialize(RayBatchSize_, HitBatchSize_);

            Framebuffer frame;
            FrameBuffer_Initialize(&frame, (uint32)camera.viewportWidth, (uint32)camera.viewportHeight, OutputLayers_);

            KernelData kernelData;
            kernelData.camera = &camera;
            kernelData.kernelCounter = 0;
            kernelData.pixelIndex = 0;
            kernelData.ptBatcher = &ptBatcher;
            kernelData.frame = &frame;
            kernelData.geometryCache = geometryCache;
            kernelData.textureCache = textureCache;
            kernelData.scene = scene;

            #if WorkerThreadCount_ > 0
                ThreadHandle threadHandles[WorkerThreadCount_];

                // -- fork threads
                for(uint scan = 0; scan < WorkerThreadCount_; ++scan) {
                    threadHandles[scan] = CreateThread(DeferredPathTracerKernel, &kernelData);
                }
            #endif

            DeferredPathTracerKernel(&kernelData);

            #if WorkerThreadCount_ > 0
                for(uint scan = 0; scan < WorkerThreadCount_; ++scan) {
                    ShutdownThread(threadHandles[scan]);
                }
            #endif

            FrameBuffer_Scale(&frame, (1.0f / (SamplesPerPixelX_ * SamplesPerPixelY_)));
            FrameBuffer_Save(&frame, imageName);
            FrameBuffer_Shutdown(&frame);

            ptBatcher.Shutdown();
        }
    }
}
