
//==============================================================================
// Joe Schutte
//==============================================================================

#include "VCM.h"
#include "VCMCommon.h"

#include "Shading/Lighting.h"
#include "Shading/SurfaceParameters.h"
#include "Shading/IntegratorContexts.h"
#include "Shading/AreaLighting.h"
#include "TextureLib/Framebuffer.h"
#include "GeometryLib/Camera.h"
#include "GeometryLib/Ray.h"
#include "GeometryLib/HashGrid.h"
#include "ThreadingLib/Thread.h"
#include "SystemLib/OSThreading.h"
#include "SystemLib/Atomic.h"
#include "SystemLib/MinMax.h"
#include "SystemLib/SystemTime.h"

#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>

#define MaxBounceCount_         10

#define EnableMultiThreading_   1
#define IntegrationSeconds_     30.0f

#define VcmRadiusFactor_ 0.0025f
#define VcmRadiusAlpha_ 0.75f

namespace Selas
{
    namespace VCM
    {
        //==============================================================================
        struct VCMKernelData
        {
            SceneContext* sceneData;
            RayCastCameraSettings camera;
            uint width;
            uint height;
            uint maxBounceCount;
            float integrationSeconds;
            std::chrono::high_resolution_clock::time_point integrationStartTime;

            float vcmRadius;
            float vcmRadiusAlpha;

            volatile int64* iterationsPerPixel;
            volatile int64* completedThreads;
            volatile int64* kernelIndices;
            volatile int64* vcmPassCount;

            Framebuffer* frame;
        };

        //==============================================================================
        bool RayPick(const RTCScene& rtcScene, const Ray& ray, HitParameters& hit)
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

            const float kErr = 32.0f * 1.19209e-07f;
            hit.error = kErr * Max(Max(Math::Absf(hit.position.x), Math::Absf(hit.position.y)), Max(Math::Absf(hit.position.z), rayhit.ray.tfar));

            hit.rxOrigin = ray.rxOrigin;
            hit.rxDirection = ray.rxDirection;
            hit.ryOrigin = ray.ryOrigin;
            hit.ryDirection = ray.ryDirection;

            return true;
        }

        //==============================================================================
        static void VertexConnectionAndMerging(GIIntegrationContext* context, CArray<VcmVertex>& pathVertices, HashGrid& hashGrid, float vmSearchRadius, uint width, uint height)
        {
            uint vmCount = width * height;
            uint vcCount = 1;

            float vmSearchRadiusSqr = vmSearchRadius * vmSearchRadius;
            float vmNormalization   = 1.0f / (Math::Pi_ * vmSearchRadiusSqr * vmCount);
            float vmWeight          = Math::Pi_ * vmSearchRadiusSqr * vmCount / vcCount;
            float vcWeight          = vcCount / (Math::Pi_ * vmSearchRadiusSqr * vmCount);

            pathVertices.Clear();
            pathVertices.Reserve((uint32)(vmCount));
            CArray<uint32> pathEnds;
            pathEnds.Reserve((uint32)(vmCount));
            CArray<float3> deletememaybe;
            deletememaybe.Reserve((uint32)(vmCount));

            // -- generate light paths
            for(uint scan = 0; scan < vmCount; ++scan) {
                
                // -- create initial light path vertex y_0 
                PathState state;
                VCMCommon::GenerateLightSample(context, vcWeight, state);

                while(state.pathLength + 2 < context->maxPathLength) {

                    // -- Make a basic ray. No differentials are used atm.
                    Ray ray = MakeRay(state.position, state.direction);

                    // -- Cast the ray against the scene
                    HitParameters hit;
                    if(RayPick(context->sceneData->rtcScene, ray, hit) == false) {
                        break;
                    }

                    // -- Calculate all surface information for this hit position
                    SurfaceParameters surface;
                    if(CalculateSurfaceParams(context, ray, &hit, surface) == false) {
                        break;
                    }

                    float connectionLengthSqr = LengthSquared(state.position - surface.position);
                    float absDotNL = Math::Absf(Dot(surface.perturbedNormal, surface.view));

                    // -- Update accumulated MIS parameters with info from our new hit position
                    if(state.pathLength > 1 || state.isAreaMeasure) {
                        state.dVCM *= connectionLengthSqr;
                    }
                    state.dVCM *= (1.0f / absDotNL);
                    state.dVC  *= (1.0f / absDotNL);
                    state.dVM  *= (1.0f / absDotNL);

                    // -- store the vertex for use with vertex merging
                    VcmVertex vcmVertex;
                    vcmVertex.throughput = state.throughput;
                    vcmVertex.pathLength = state.pathLength;
                    vcmVertex.dVCM = state.dVCM;
                    vcmVertex.dVC = state.dVC;
                    vcmVertex.dVM = state.dVM;
                    vcmVertex.surface = surface;
                    pathVertices.Add(vcmVertex);
                    deletememaybe.Add(surface.position);

                    // -- connect the path to the camera
                    VCMCommon::ConnectLightPathToCamera(context, state, surface, vmWeight, (float)vmCount);

                    // -- bsdf scattering to advance the path
                    if(VCMCommon::SampleBsdfScattering(context, surface, vmWeight, vcWeight, state) == false) {
                        break;
                    }
                }

                pathEnds.Add(pathVertices.Length());
            }

            // -- build the hash grid
            BuildHashGrid(&hashGrid, vmCount, vmSearchRadius, deletememaybe);

            // -- generate camera paths
            for(uint y = 0; y < height; ++y) {
                for(uint x = 0; x < width; ++x) {
                    uint index = y * width + x;

                    PathState cameraPathState;
                    VCMCommon::GenerateCameraSample(context, x, y, (float)vmCount, cameraPathState);

                    float3 color = float3::Zero_;

                    while(cameraPathState.pathLength < context->maxPathLength) {

                       // -- Make a basic ray. No differentials are used atm...
                        Ray ray = MakeRay(cameraPathState.position, cameraPathState.direction);

                        // -- Cast the ray against the scene
                        HitParameters hit;
                        if(RayPick(context->sceneData->rtcScene, ray, hit) == false) {
                            // -- if the ray exits the scene then we sample the ibl and accumulate the results.
                            float3 sample = cameraPathState.throughput * VCMCommon::ConnectToSkyLight(context, cameraPathState);
                            color += sample;
                            break;
                        }

                        // -- Calculate all surface information for this hit position
                        SurfaceParameters surface;
                        if(CalculateSurfaceParams(context, ray, &hit, surface) == false) {
                            break;
                        }

                        float connectionLengthSqr = LengthSquared(cameraPathState.position - surface.position);
                        float absDotNL = Math::Absf(Dot(surface.geometricNormal, surface.view));

                        // -- Update accumulated MIS parameters with info from our new hit position. This combines with work done at the previous vertex to 
                        // -- convert the solid angle pdf to the area pdf of the outermost term.
                        cameraPathState.dVCM *= connectionLengthSqr;
                        cameraPathState.dVCM /= absDotNL;
                        cameraPathState.dVC  /= absDotNL;
                        cameraPathState.dVM  /= absDotNL;

                        // -- Vertex connection to a light source
                        if(cameraPathState.pathLength + 1 < context->maxPathLength) {
                            float3 sample = cameraPathState.throughput * VCMCommon::ConnectCameraPathToLight(context, cameraPathState, surface, vmWeight);
                            color += sample; 
                        }

                        // -- Vertex connection to a light vertex
                        {
                            uint pathStart = (index == 0) ? 0 : pathEnds[index - 1];
                            uint pathEnd = pathEnds[index];

                            for(uint lightVertexIndex = pathStart; lightVertexIndex < pathEnd; ++lightVertexIndex) {
                                const VcmVertex& lightVertex = pathVertices[lightVertexIndex];
                                if(lightVertex.pathLength + 1 + cameraPathState.pathLength > context->maxPathLength) {
                                    break;
                                }

                                color += cameraPathState.throughput * lightVertex.throughput * VCMCommon::ConnectPathVertices(context, surface, cameraPathState, lightVertex, vmWeight);
                            }
                        }

                        // -- Vertex merging
                        {
                            VertexMergingCallbackStruct callbackData;
                            callbackData.context = context;
                            callbackData.surface = &surface;
                            callbackData.pathVertices = &pathVertices;
                            callbackData.cameraState = &cameraPathState;
                            callbackData.vcWeight = vcWeight;
                            callbackData.result = float3::Zero_;
                            SearchHashGrid(&hashGrid, deletememaybe, surface.position, &callbackData, VCMCommon::MergeVertices);

                            color += cameraPathState.throughput * vmNormalization * callbackData.result;
                        }

                        // -- bsdf scattering to advance the path
                        if(VCMCommon::SampleBsdfScattering(context, surface, vmWeight, vcWeight, cameraPathState) == false) {
                            break;
                        }
                    }

                    FramebufferWriter_Write(&context->frameWriter, color, (uint32)x, (uint32)y);
                }
            }

            // -- debug tech
            //for(uint scan = 0, count = pathVertices.Length(); scan < count; ++scan) {
            //    const SurfaceParameters& surface = pathVertices[scan].surface;

            //    float3 toPosition = surface.position - context->camera->position;
            //    if(Dot(context->camera->forward, toPosition) > 0.0f) {
            //        int2 imagePosition = WorldToImage(context->camera, surface.position);
            //        if(imagePosition.x >= 0 && imagePosition.x < context->camera->viewportWidth && imagePosition.y > 0 && imagePosition.y < context->camera->viewportHeight) {

            //            float3 color;
            //            if(pathVertices[scan].pathLength == 1)
            //                color = float3(0.0f, 0.0f, 10.0f);
            //            else if(pathVertices[scan].pathLength >= 2 && pathVertices[scan].pathLength < 3)
            //                color = float3(0.0f, 10.0f, 0.0f);
            //            else
            //                color = float3(10.0f, 0.0f, 0.0f);
            //            uint index = imagePosition.y * context->imageWidth + imagePosition.x;
            //            context->imageData[index] = context->imageData[index] + color;
            //        }
            //    }
            //}
        }

        //==============================================================================
        static void VCMKernel(void* userData)
        {
            VCMKernelData* vcmKernelData = static_cast<VCMKernelData*>(userData);
            int64 kernelIndex = Atomic::Increment64(vcmKernelData->kernelIndices);

            Random::MersenneTwister twister;
            Random::MersenneTwisterInitialize(&twister, (uint32)kernelIndex);

            uint width = vcmKernelData->width;
            uint height = vcmKernelData->height;

            GIIntegrationContext context;
            context.sceneData        = vcmKernelData->sceneData;
            context.camera           = &vcmKernelData->camera;
            context.imageWidth       = width;
            context.imageHeight      = height;
            context.twister          = &twister;
            context.maxPathLength    = vcmKernelData->maxBounceCount;

            FramebufferWriter_Initialize(&context.frameWriter, vcmKernelData->frame,
                                         DefaultFrameWriterCapacity_, DefaultFrameWriterSoftCapacity_);

            HashGrid hashGrid;
            CArray<VcmVertex> lightVertices;

            int64 pathsTracedPerPixel = 0;
            float elapsedSeconds = 0.0f;
            while(elapsedSeconds < vcmKernelData->integrationSeconds) {
                int64 index = Atomic::Increment64(vcmKernelData->vcmPassCount);
                float iterationIndex = index + 1.0f;

                float vcmKernelRadius = vcmKernelData->vcmRadius / Math::Powf(iterationIndex, 0.5f * (1.0f - vcmKernelData->vcmRadiusAlpha));

                VertexConnectionAndMerging(&context, lightVertices, hashGrid, vcmKernelRadius, width, height);
                ++pathsTracedPerPixel;

                elapsedSeconds = SystemTime::ElapsedSecondsF(vcmKernelData->integrationStartTime);
            }

            ShutdownHashGrid(&hashGrid);
            lightVertices.Close();

            Atomic::Add64(vcmKernelData->iterationsPerPixel, pathsTracedPerPixel);

            Random::MersenneTwisterShutdown(&twister);

            FramebufferWriter_Shutdown(&context.frameWriter);
            Atomic::Increment64(vcmKernelData->completedThreads);
        }

        //==============================================================================
        void GenerateImage(SceneContext& context, Framebuffer* frame)
        {
            const SceneResource* scene = context.scene;
            SceneMetaData* sceneData = scene->data;

            uint width = frame->width;
            uint height = frame->height;

            RayCastCameraSettings camera;
            InitializeRayCastCamera(scene->data->camera, width, height, camera);

            int64 completedThreads = 0;
            int64 kernelIndex = 0;
            int64 pathsEvaluatedPerPixel = 0;
            int64 vcmPassCount = 0;

            #if EnableMultiThreading_ 
                const uint additionalThreadCount = 7;
            #else
                const uint additionalThreadCount = 0;
            #endif

            VCMKernelData integratorContext;
            integratorContext.sceneData              = &context;
            integratorContext.camera                 = camera;
            integratorContext.frame                  = frame;
            integratorContext.width                  = width;
            integratorContext.height                 = height;
            integratorContext.maxBounceCount         = MaxBounceCount_;
            integratorContext.integrationStartTime   = SystemTime::Now();
            integratorContext.integrationSeconds     = IntegrationSeconds_;
            integratorContext.iterationsPerPixel     = &pathsEvaluatedPerPixel;
            integratorContext.vcmPassCount           = &vcmPassCount;
            integratorContext.completedThreads       = &completedThreads;
            integratorContext.kernelIndices          = &kernelIndex;
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

            FrameBuffer_Normalize(frame, (1.0f / pathsEvaluatedPerPixel));
        }
    }
}