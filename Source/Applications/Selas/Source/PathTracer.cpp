
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
#include "SystemLib/CountOf.h"

#include "embree3/rtcore.h"
#include "embree3/rtcore_ray.h"

#define MaxBounceCount_         2048

#define AdditionalThreadCount_  6
#define PathsPerPixel_          16
#define LayerCount_             2

namespace Selas
{
    namespace PathTracer
    {
        //=========================================================================================================================
        struct PathTracingKernelData
        {
            GeometryCache* geometryCache;
            TextureCache* textureCache;
            SceneResource* scene;
            RayCastCameraSettings camera;
            uint pathsPerPixel;
            uint maxBounceCount;
            std::chrono::high_resolution_clock::time_point integrationStartTime;

            volatile uint64* pixelIndex;
            volatile int64* completedThreads;
            volatile int64* kernelIndices;

            Framebuffer* frame;
        };

        //=========================================================================================================================
        static bool OcclusionRay(const RTCScene& rtcScene, const SurfaceParameters& surface, float3 direction, float distance)
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

        //=========================================================================================================================
        static bool RayPick(const RTCScene& rtcScene, const Ray& ray, float tfar, HitParameters& hit)
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
            rayhit.ray.tnear = 0.0f;
            rayhit.ray.tfar = tfar;

            rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
            rayhit.hit.primID = RTC_INVALID_GEOMETRY_ID;
            rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
            rayhit.hit.instID[1] = RTC_INVALID_GEOMETRY_ID;

            rtcIntersect1(rtcScene, &context, &rayhit);

            if(rayhit.hit.geomID == -1)
                return false;

            hit.position.x = rayhit.ray.org_x + rayhit.ray.tfar * ray.direction.x;
            hit.position.y = rayhit.ray.org_y + rayhit.ray.tfar * ray.direction.y;
            hit.position.z = rayhit.ray.org_z + rayhit.ray.tfar * ray.direction.z;
            hit.normal.x = rayhit.hit.Ng_x;
            hit.normal.y = rayhit.hit.Ng_y;
            hit.normal.z = rayhit.hit.Ng_z;
            hit.baryCoords = { rayhit.hit.u, rayhit.hit.v };
            hit.geomId = rayhit.hit.geomID;
            hit.primId = rayhit.hit.primID;
            hit.instId[0] = rayhit.hit.instID[0];
            hit.instId[1] = rayhit.hit.instID[1];
            hit.incDirection = -ray.direction;

            const float kErr = 32.0f * 1.19209e-07f;
            hit.error = kErr * Max(Max(Math::Absf(hit.position.x), Math::Absf(hit.position.y)), Max(Math::Absf(hit.position.z),
                                                                                                               rayhit.ray.tfar));

            return true;
        }

        //=========================================================================================================================
        static void EvaluatePath(GIIntegratorContext* __restrict context, Ray ray, uint x, uint y)
        {
            float3 Ld[LayerCount_];
            Memory::Zero(Ld, sizeof(Ld));

            float3 throughput = float3::One_;

            MediumParameters vacuum;
            MediumParameters currentMedium = vacuum;

            float rayDistance = FloatMax_;

            float isDeltaOnly = true;

            uint bounceCount = 0;
            while (bounceCount < context->maxPathLength) {
                
                float pdf;
                rayDistance = SampleDistance(&context->sampler, currentMedium, &pdf);

                HitParameters hit;
                bool rayCastHit = RayPick(context->rtcScene, ray, rayDistance, hit);

                if(rayCastHit) {
                    rayDistance = Length(hit.position - ray.origin);
                }
                float3 transmission = Transmission(currentMedium, rayDistance);
                throughput = throughput * transmission;

                if(rayCastHit) {
                    SurfaceParameters surface;
                    if(CalculateSurfaceParams(context, &hit, surface) == false) {
                        break;
                    }

                    // -- choose a light and sample the light source
                    LightDirectSample lightSample;
                    NextEventEstimation(context, hit.position, GeometricNormal(surface), lightSample);

                    if(Dot(lightSample.radiance, float3::One_) > 0) {
                        float forwardPdfW;
                        float reversePdfW;
                        float3 reflectance = EvaluateBsdf(surface, -ray.direction, lightSample.direction, forwardPdfW,
                                                          reversePdfW);
                        if(Dot(reflectance, float3::One_) > 0) {

                            if(OcclusionRay(context->scene->rtcScene, surface, lightSample.direction, lightSample.distance)) {

                                float3 sample = reflectance * lightSample.radiance * (1.0f / lightSample.pdfW);
                                Ld[1] += sample * throughput;
                            }
                        }
                    }


                    {
                        // - sample the bsdf
                        BsdfSample bsdfSample;
                        if(SampleBsdfFunction(&context->sampler, surface, -ray.direction, bsdfSample) == false) {
                            break;
                        }

                        isDeltaOnly = isDeltaOnly && (bsdfSample.flags & SurfaceEventFlags::eDeltaEvent);

                        if(bsdfSample.flags == SurfaceEventFlags::eTransmissionEvent) {
                            // -- Currently we only support "air"(vacuum)-surface interfaces without any nesting.
                            if(currentMedium.phaseFunction == eVacuum)
                                currentMedium = bsdfSample.medium;
                            else
                                currentMedium = vacuum;
                        }

                        float lightPdfW = LightingPdf(context, lightSample, surface.position, bsdfSample.wi);
                        float weight = 1.0f;// ImportanceSampling::BalanceHeuristic(1, bsdfSample.forwardPdfW, 1, lightPdfW);

                        throughput = weight * throughput * bsdfSample.reflectance;

                        float3 offsetOrigin = OffsetRayOrigin(surface, bsdfSample.wi, 1.0f);
                        ray = MakeRay(offsetOrigin, bsdfSample.wi);

                        if(bounceCount == 0) {
                            Ld[0] = bsdfSample.reflectance;
                        }
                    }
                }
                else if(currentMedium.phaseFunction != MediumPhaseFunction::eVacuum) {
                    float3 origin = ray.origin + rayDistance * ray.direction;
                    float mediumPdf;
                    float3 direction = SampleScatterDirection(&context->sampler, currentMedium, ray.direction, &mediumPdf);
                    ray = MakeRay(origin, direction);
                }
                else {
                    float3 sample;
                    if(isDeltaOnly)
                        sample = SampleBackgroundMiss(context, ray.direction);
                    else
                        sample = SampleBackground(context, ray.direction);

                    Ld[1] += sample * throughput;
                    break;
                }

                ++bounceCount;

                // --Russian roulette path termination
                if(bounceCount > 8) {
                    float continuationProb = Max<float>(Max<float>(throughput.x, throughput.y), throughput.z);
                    if(context->sampler.UniformFloat() > continuationProb) {
                        break;
                    }
                    throughput = throughput * (1.0f / continuationProb);
                }
            }

            FramebufferWriter_Write(&context->frameWriter, Ld, LayerCount_, (uint32)x, (uint32)y);
        }

        //=========================================================================================================================
        static void PathTracerKernel(void* userData)
        {
            PathTracingKernelData* integratorContext = static_cast<PathTracingKernelData*>(userData);
            int64 kernelIndex = Atomic::Increment64(integratorContext->kernelIndices);

            uint pathsPerPixel = integratorContext->pathsPerPixel;

            uint width = integratorContext->camera.width;
            uint height = integratorContext->camera.height;
            uint64 totalPixelCount = width * height;

            GIIntegratorContext context;
            context.geometryCache    = integratorContext->geometryCache;
            context.textureCache     = integratorContext->textureCache;
            context.rtcScene         = integratorContext->scene->rtcScene;
            context.scene            = integratorContext->scene;
            context.camera           = &integratorContext->camera;
            context.sampler.Initialize((uint32)kernelIndex);
            context.maxPathLength    = integratorContext->maxBounceCount;
            FramebufferWriter_Initialize(&context.frameWriter, integratorContext->frame,
                                         DefaultFrameWriterCapacity_, DefaultFrameWriterSoftCapacity_);

            while(*integratorContext->pixelIndex < totalPixelCount) {

                uint64 pixelIndex = Atomic::AddU64(integratorContext->pixelIndex, 1llu) % totalPixelCount;
                uint y = pixelIndex / width;
                uint x = pixelIndex - y * width;

                for(uint scan = 0; scan < pathsPerPixel; ++scan) {
                    Ray ray = JitteredCameraRay(context.camera, &context.sampler, (float)x, (float)y);
                    EvaluatePath(&context, ray, x, y);
                }
            }

            context.sampler.Shutdown();

            FramebufferWriter_Shutdown(&context.frameWriter);
            Atomic::Increment64(integratorContext->completedThreads);
        }

        //=========================================================================================================================
        void GenerateImage(GeometryCache* geometryCache, TextureCache* textureCache, SceneResource* scene,
                           const RayCastCameraSettings& camera, cpointer imageName)
        {
            Framebuffer frame;
            FrameBuffer_Initialize(&frame, (uint32)camera.viewportWidth, (uint32)camera.viewportHeight, LayerCount_);

            int64 completedThreads = 0;
            int64 kernelIndex = 0;
            //uint64 pixelIndex = 409600;
            uint64 pixelIndex = 0;

            PathTracingKernelData integratorContext;
            integratorContext.geometryCache          = geometryCache;
            integratorContext.textureCache           = textureCache;
            integratorContext.scene                  = scene;
            integratorContext.camera                 = camera;
            integratorContext.maxBounceCount         = MaxBounceCount_;
            integratorContext.pathsPerPixel          = PathsPerPixel_;
            integratorContext.integrationStartTime   = SystemTime::Now();
            integratorContext.pixelIndex             = &pixelIndex;
            integratorContext.completedThreads       = &completedThreads;
            integratorContext.kernelIndices          = &kernelIndex;
            integratorContext.frame                  = &frame;

            #if AdditionalThreadCount_ > 0
                ThreadHandle threadHandles[AdditionalThreadCount_];

                // -- fork threads
                for(uint scan = 0; scan < AdditionalThreadCount_; ++scan) {
                    threadHandles[scan] = CreateThread(PathTracerKernel, &integratorContext);
                }
            #endif

            // -- do work on the main thread too
            PathTracerKernel(&integratorContext);

            #if AdditionalThreadCount_ > 0
                // -- wait for any other threads to finish
                while(*integratorContext.completedThreads != *integratorContext.kernelIndices);

                for(uint scan = 0; scan < AdditionalThreadCount_; ++scan) {
                    ShutdownThread(threadHandles[scan]);
                }
            #endif

            FrameBuffer_Normalize(&frame, (1.0f / PathsPerPixel_));

            FrameBuffer_Save(&frame, imageName);
            FrameBuffer_Shutdown(&frame);
        }
    }
}
