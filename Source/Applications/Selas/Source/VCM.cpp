//
////=================================================================================================================================
//// Joe Schutte
////=================================================================================================================================
//
//#include "VCM.h"
//#include "VCMCommon.h"
//#include "VCMHashGrid.h"
//
//#include "SceneLib/SceneResource.h"
//#include "Shading/SurfaceScattering.h"
//#include "Shading/SurfaceParameters.h"
//#include "Shading/IntegratorContexts.h"
//#include "Shading/AreaLighting.h"
//#include "TextureLib/Framebuffer.h"
//#include "GeometryLib/Camera.h"
//#include "GeometryLib/Ray.h"
//#include "ThreadingLib/Thread.h"
//#include "SystemLib/OSThreading.h"
//#include "SystemLib/Atomic.h"
//#include "SystemLib/MinMax.h"
//#include "SystemLib/SystemTime.h"
//#include "SystemLib/Profiling.h"
//#include "SystemLib/Logging.h"
//
//#include "embree3/rtcore.h"
//#include "embree3/rtcore_ray.h"
//
//#define MaxBounceCount_         10
//
//#define EnableMultiThreading_   1
//#define IntegrationSeconds_     60.0f
//
//#define VcmRadiusFactor_ 0.0025f
//#define VcmRadiusAlpha_ 0.75f
//
//namespace Selas
//{
//    namespace VCM
//    {
//        //=========================================================================================================================
//        struct VCMSharedData
//        {
//            RTCScene rtcScene;
//            SceneResource* scene;
//            RayCastCameraSettings camera;
//            uint maxBounceCount;
//            float integrationSeconds;
//            std::chrono::high_resolution_clock::time_point integrationStartTime;
//
//            float vcmRadius;
//            float vcmRadiusAlpha;
//
//            volatile int64* iterationsPerPixel;
//            volatile int64* completedThreads;
//            volatile int64* kernelIndices;
//            volatile int64* vcmPassCount;
//
//            Framebuffer* frame;
//        };
//
//        struct LightPathSet
//        {
//            VCMHashGrid hashGrid;
//            CArray<VCMVertex> lightVertices;
//            CArray<uint64> pathEnds;
//        };
//
//        //=========================================================================================================================
//        static bool OcclusionRay(RTCScene& rtcScene, const SurfaceParameters& surface, float3 direction, float distance)
//        {
//            ProfileEventMarker_(0x88FFFFFF, "OcclusionRay");
//
//            float3 origin = OffsetRayOrigin(surface, direction, 0.1f);
//
//            RTCIntersectContext context;
//            rtcInitIntersectContext(&context);
//
//            Align_(16) RTCRay ray;
//            ray.org_x = origin.x;
//            ray.org_y = origin.y;
//            ray.org_z = origin.z;
//            ray.dir_x = direction.x;
//            ray.dir_y = direction.y;
//            ray.dir_z = direction.z;
//            ray.tnear = surface.error;
//            ray.tfar = distance;
//
//            rtcOccluded1(rtcScene, &context, &ray);
//
//            // -- ray.tfar == -inf when hit occurs
//            return (ray.tfar >= 0.0f);
//        }
//
//        //=========================================================================================================================
//        static bool VcOcclusionRay(RTCScene& rtcScene, const SurfaceParameters& surface, float3 direction, float distance)
//        {
//            ProfileEventMarker_(0x88FFFFFF, "VcOcclusionRay");
//
//            float biasDistance;
//            float3 origin = OffsetRayOrigin(surface, direction, 0.1f, biasDistance);
//
//            RTCIntersectContext context;
//            rtcInitIntersectContext(&context);
//
//            Align_(16) RTCRay ray;
//            ray.org_x = origin.x;
//            ray.org_y = origin.y;
//            ray.org_z = origin.z;
//            ray.dir_x = direction.x;
//            ray.dir_y = direction.y;
//            ray.dir_z = direction.z;
//            ray.tnear = surface.error;
//            ray.tfar = distance - 16.0f * Math::Absf(biasDistance);
//
//            rtcOccluded1(rtcScene, &context, &ray);
//
//            // -- ray.tfar == -inf when hit occurs
//            return (ray.tfar >= 0.0f);
//        }
//
//        //=========================================================================================================================
//        static bool RayPick(const RTCScene& rtcScene, const Ray& ray, HitParameters& hit)
//        {
//            ProfileEventMarker_(0x88FFFFFF, "RayPick");
//
//            RTCIntersectContext context;
//            rtcInitIntersectContext(&context);
//
//            Align_(16) RTCRayHit rayhit;
//            rayhit.ray.org_x = ray.origin.x;
//            rayhit.ray.org_y = ray.origin.y;
//            rayhit.ray.org_z = ray.origin.z;
//            rayhit.ray.dir_x = ray.direction.x;
//            rayhit.ray.dir_y = ray.direction.y;
//            rayhit.ray.dir_z = ray.direction.z;
//            rayhit.ray.tnear = 0.00001f;
//            rayhit.ray.tfar = FloatMax_;
//
//            rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
//            rayhit.hit.primID = RTC_INVALID_GEOMETRY_ID;
//            rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
//            rayhit.hit.instID[1] = RTC_INVALID_GEOMETRY_ID;
//
//            rtcIntersect1(rtcScene, &context, &rayhit);
//
//            if(rayhit.hit.geomID == -1)
//                return false;
//
//            hit.position.x = rayhit.ray.org_x + rayhit.ray.tfar * ray.direction.x;
//            hit.position.y = rayhit.ray.org_y + rayhit.ray.tfar * ray.direction.y;
//            hit.position.z = rayhit.ray.org_z + rayhit.ray.tfar * ray.direction.z;
//            hit.normal.x = rayhit.hit.Ng_x;
//            hit.normal.y = rayhit.hit.Ng_y;
//            hit.normal.z = rayhit.hit.Ng_z;
//            hit.baryCoords = { rayhit.hit.u, rayhit.hit.v };
//            hit.geomId = rayhit.hit.geomID;
//            hit.primId = rayhit.hit.primID;
//            hit.instId[0] = rayhit.hit.instID[0];
//            hit.instId[1] = rayhit.hit.instID[1];
//            hit.incDirection = -ray.direction;
//
//            const float kErr = 32.0f * 1.19209e-07f;
//            hit.error = kErr * Max(Max(Math::Absf(hit.position.x), Math::Absf(hit.position.y)),
//                                   Max(Math::Absf(hit.position.z), rayhit.ray.tfar));
//
//            return true;
//        }
//
//        //=========================================================================================================================
//        static float3 ConnectToSkyLight(GIIntegratorContext* context, PathState& state)
//        {
//            ProfileEventMarker_(0x88FFFFFF, "ConnectToSkyLight");
//
//            float directPdfA;
//            float emissionPdfW;
//            float3 radiance = IblCalculateRadiance(context, state.direction, directPdfA, emissionPdfW);
//
//            if(state.pathLength == 1) {
//                return radiance;
//            }
//
//            float cameraWeight = directPdfA * state.dVCM + emissionPdfW * state.dVC;
//            float misWeight = 1.0f / (1.0f + cameraWeight);
//
//            return misWeight * radiance;
//        }
//
//        //=========================================================================================================================
//        static void ConnectLightPathToCamera(GIIntegratorContext* context, PathState& state, const SurfaceParameters& surface,
//                                             float vmWeight, float lightPathCount)
//        {
//            ProfileEventMarker_(0x88FFFFFF, "ConnectLightPathToCamera");
//
//            const RayCastCameraSettings* __restrict camera = context->camera;
//
//            float3 toPosition = surface.position - camera->position;
//            if(Dot(camera->forward, toPosition) <= 0.0f) {
//                return;
//            }
//
//            int2 imagePosition = WorldToImage(camera, surface.position);
//            if(imagePosition.x < 0 || imagePosition.x >= (int32)camera->width || imagePosition.y < 0 
//               || imagePosition.y >= (int32)camera->height) {
//                return;
//            }
//
//            float distance = Length(toPosition);
//            toPosition = (1.0f / distance) * toPosition;
//
//            float cosThetaCamera = Dot(camera->forward, toPosition);
//
//            float imagePointToCameraDistance = camera->virtualImagePlaneDistance / cosThetaCamera;
//            float imageToSolidAngle = imagePointToCameraDistance * imagePointToCameraDistance / cosThetaCamera;
//            float imageToSurface = imageToSolidAngle * cosThetaCamera;
//            float surfaceToImage = 1.0f / imageToSurface;
//            float cameraPdfA = imageToSurface;
//
//            // -- evaluate BSDF
//            float bsdfForwardPdf;
//            float bsdfReversePdf;
//            float3 bsdf = EvaluateBsdf(surface, -state.direction, -toPosition, bsdfForwardPdf, bsdfReversePdf);
//            if(bsdf.x == 0 && bsdf.y == 0 && bsdf.z == 0) {
//                return;
//            }
//
//            float constProb = ContinuationProbability(surface);
//            bsdfForwardPdf *= constProb;
//            bsdfReversePdf *= constProb;
//
//            float lightPartialWeight = (cameraPdfA / lightPathCount) * (vmWeight + state.dVCM + state.dVC * bsdfReversePdf);
//            float misWeight = 1.0f / (lightPartialWeight + 1.0f);
//
//            float3 pathContribution = misWeight * state.throughput * bsdf * (1.0f / (lightPathCount * surfaceToImage));
//            if(pathContribution.x == 0 && pathContribution.y == 0 && pathContribution.z == 0) {
//                return;
//            }
//
//            if(OcclusionRay(context->rtcScene, surface, -toPosition, distance)) {
//                FramebufferWriter_Write(&context->frameWriter, &pathContribution, 1, imagePosition.x, imagePosition.y);
//            }
//        }
//
//        //=========================================================================================================================
//        static float3 ConnectCameraPathToLight(GIIntegratorContext* context, PathState& state, const SurfaceParameters& surface,
//                                               float vmWeight)
//        {
//            ProfileEventMarker_(0x88FFFFFF, "ConnectCameraPathToLight");
//
//            // -- only using the ibl for now
//            float lightSampleWeight = 1.0f;
//
//            // -- choose direction to sample the ibl
//
//            LightDirectSample sample;
//            {
//                // -- JSTODO - Sample area lights and such
//                DirectIblLightSample(context, sample);
//                sample.directionPdfA *= lightSampleWeight;
//            }
//
//            float bsdfForwardPdfW;
//            float bsdfReversePdfW;
//            float3 bsdf = EvaluateBsdf(surface, -state.direction, sample.direction, bsdfForwardPdfW, bsdfReversePdfW);
//            if(bsdf.x == 0 && bsdf.y == 0 && bsdf.z == 0) {
//                return float3::Zero_;
//            }
//
//            float contProb = ContinuationProbability(surface);
//            bsdfForwardPdfW *= contProb;
//            bsdfReversePdfW *= contProb;
//
//            float cosThetaSurface = Math::Absf(Dot(surface.perturbedNormal, sample.direction));
//
//            float lightWeight = bsdfForwardPdfW / sample.directionPdfA;
//            float cameraWeight = (sample.emissionPdfW * cosThetaSurface / (sample.directionPdfA * sample.cosThetaLight))
//                               * (vmWeight + state.dVCM + state.dVC * bsdfReversePdfW);
//            float misWeight = 1.0f / (lightWeight + 1 + cameraWeight);
//
//            float3 pathContribution = (misWeight * cosThetaSurface / sample.directionPdfA) * sample.radiance * bsdf;
//            if(pathContribution.x == 0 && pathContribution.y == 0 && pathContribution.z == 0) {
//                return float3::Zero_;
//            }
//
//            if(OcclusionRay(context->rtcScene, surface, sample.direction, sample.distance)) {
//                return pathContribution;
//            }
//
//            return float3::Zero_;
//        }
//
//        //=========================================================================================================================
//        static float3 ConnectPathVertices(GIIntegratorContext* context, const SurfaceParameters& surface,
//                                          const PathState& cameraState, const VCMVertex& lightVertex, float vmWeight)
//        {
//            ProfileEventMarker_(0x88FFFFFF, "ConnectPathVertices");
//
//            float3 direction = lightVertex.hit.position - surface.position;
//            float distanceSquared = LengthSquared(direction);
//            float distance = Math::Sqrtf(distanceSquared);
//            direction = (1.0f / distance) * direction;
//
//            float cameraBsdfForwardPdfW;
//            float cameraBsdfReversePdfW;
//            float3 cameraBsdf = EvaluateBsdf(surface, -cameraState.direction, direction, cameraBsdfForwardPdfW,
//                                             cameraBsdfReversePdfW);
//            if(cameraBsdf.x == 0 && cameraBsdf.y == 0 && cameraBsdf.z == 0) {
//                return float3::Zero_;
//            }
//
//            SurfaceParameters lightSurface;
//            if(CalculateSurfaceParams(context, &lightVertex.hit, lightSurface) == false) {
//                return float3::Zero_;
//            }
//
//            float lightBsdfForwardPdfW;
//            float lightBsdfReversePdfW;
//            float3 lightBsdf = EvaluateBsdf(lightSurface, -direction, lightSurface.view, lightBsdfForwardPdfW,
//                                            lightBsdfReversePdfW);
//            if(lightBsdf.x == 0 && lightBsdf.y == 0 && lightBsdf.z == 0) {
//                return float3::Zero_;
//            }
//
//            float cosThetaCamera = Math::Absf(Dot(direction, surface.perturbedNormal));
//            float cosThetaLight = Math::Absf(Dot(-direction, lightSurface.perturbedNormal));
//
//            float geometryTerm = cosThetaLight * cosThetaCamera / distanceSquared;
//            if(geometryTerm < 0.0f) {
//                // -- JSTODO - For this to be possible the cosTheta terms would need to be negative. But with transparent
//                // -- surfaces the normal will often be for the other side of the surface.
//                return float3::Zero_;
//            }
//
//            // -- russian roulette
//            float cameraContProb = ContinuationProbability(surface);
//            cameraBsdfForwardPdfW *= cameraContProb;
//            cameraBsdfReversePdfW *= cameraContProb;
//            float lightContProb = ContinuationProbability(lightSurface);
//            lightBsdfForwardPdfW *= lightContProb;
//            lightBsdfReversePdfW *= lightContProb;
//
//            // -- convert pdfs from solid angle to area measure
//            float cameraBsdfPdfA = cameraBsdfForwardPdfW * Math::Absf(cosThetaLight) / distanceSquared;
//            float lightBsdfPdfA = lightBsdfForwardPdfW * Math::Absf(cosThetaCamera) / distanceSquared;
//            float lightWeight = cameraBsdfPdfA * (vmWeight + lightVertex.dVCM + lightVertex.dVC * lightBsdfReversePdfW);
//            float cameraWeight = lightBsdfPdfA * (vmWeight + cameraState.dVCM + cameraState.dVC * cameraBsdfReversePdfW);
//            float misWeight = 1.0f / (lightWeight + 1.0f + cameraWeight);
//
//            float3 pathContribution = misWeight * geometryTerm * cameraBsdf * lightBsdf;
//            if(pathContribution.x == 0 && pathContribution.y == 0 && pathContribution.z == 0) {
//                return float3::Zero_;
//            }
//
//            if(VcOcclusionRay(context->rtcScene, surface, direction, distance)) {
//                return pathContribution;
//            }
//
//            return float3::Zero_;
//        }
//
//        //=========================================================================================================================
//        static void MergeVertices(const VCMVertex& lightVertex, void* userData)
//        {
//            ProfileEventMarker_(0x88FFFFFF, "MergeVertices");
//
//            VertexMergingCallbackStruct* vmData = (VertexMergingCallbackStruct*)userData;
//            const GIIntegratorContext* context = vmData->context;
//            const SurfaceParameters& surface = *vmData->surface;
//            const PathState& cameraState = *vmData->cameraState;
//
//            if(cameraState.pathLength + lightVertex.pathLength > context->maxPathLength) {
//                return;
//            }
//
//            SurfaceParameters lightSurface;
//            if(CalculateSurfaceParams(context, &lightVertex.hit, lightSurface) == false) {
//                return;
//            }
//
//            // JSTODO - This is a hack :(. How do I correctly prevent ringing around the base of glossy transparent objects?
//            if((surface.materialFlags & eTransparent) != (lightSurface.materialFlags & eTransparent)) {
//                return;
//            }
//
//            float bsdfForwardPdfW;
//            float bsdfReversePdfW;
//            float3 bsdf = EvaluateBsdf(surface, -cameraState.direction, lightSurface.view, bsdfForwardPdfW, bsdfReversePdfW);
//            if(bsdf.x == 0 && bsdf.y == 0 && bsdf.z == 0) {
//                return;
//            }
//
//            bsdfForwardPdfW *= ContinuationProbability(surface);
//            bsdfReversePdfW *= ContinuationProbability(lightSurface);
//
//            float lightWeight = lightVertex.dVCM * vmData->vcWeight + lightVertex.dVM * bsdfForwardPdfW;
//            float cameraWeight = cameraState.dVCM * vmData->vcWeight + cameraState.dVM * bsdfReversePdfW;
//            float misWeight = 1.0f / (lightWeight + 1.0f + cameraWeight);
//
//            #if CheckForNaNs_
//                Assert_(!Math::IsNaN(bsdf.x));
//                Assert_(!Math::IsNaN(bsdf.y));
//                Assert_(!Math::IsNaN(bsdf.z));
//                Assert_(!Math::IsNaN(lightVertex.throughput.x));
//                Assert_(!Math::IsNaN(lightVertex.throughput.y));
//                Assert_(!Math::IsNaN(lightVertex.throughput.z));
//            #endif
//
//            vmData->result += misWeight * bsdf * lightVertex.throughput;
//        }
//
//        //=========================================================================================================================
//        static bool SampleBsdfScattering(CSampler* sampler, const SurfaceParameters& surface, float vmWeight, float vcWeight,
//                                         PathState& pathState)
//        {
//            ProfileEventMarker_(0x88FFFFFF, "SampleBsdfScattering");
//
//            BsdfSample sample;
//            if(SampleBsdfFunction(sampler, surface, -pathState.direction, sample) == false) {
//                return false;
//            }
//            if(sample.reflectance.x == 0.0f && sample.reflectance.y == 0.0f && sample.reflectance.z == 0.0f) {
//                return false;
//            }
//
//            float contProb = ContinuationProbability(surface);
//            float russianRouletteSample = sampler->UniformFloat();
//            if(russianRouletteSample > contProb) {
//                return false;
//            }
//            sample.forwardPdfW *= contProb;
//            sample.reversePdfW *= contProb;
//
//            float cosThetaBsdf = Math::Absf(Dot(sample.wi, surface.perturbedNormal));
//
//            pathState.position = surface.position;
//            pathState.throughput = pathState.throughput * sample.reflectance;
//            pathState.dVC = (cosThetaBsdf / sample.forwardPdfW) 
//                          * (pathState.dVC * sample.reversePdfW + pathState.dVCM + vmWeight);
//            pathState.dVM = (cosThetaBsdf / sample.forwardPdfW) 
//                          * (pathState.dVM * sample.reversePdfW + pathState.dVCM * vcWeight + 1.0f);
//            pathState.dVCM = 1.0f / sample.forwardPdfW;
//            pathState.direction = sample.wi;
//            ++pathState.pathLength;
//
//            return true;
//        }
//
//        //=========================================================================================================================
//        static bool EvaluateLightPathHit(const VCMIterationConstants& constants, GIIntegratorContext* context,
//                                         PathState& state, HitParameters& hit, LightPathSet* lightPathSet)
//        {
//            // -- Calculate all surface information for this hit position
//            SurfaceParameters surface;
//            if(CalculateSurfaceParams(context, &hit, surface) == false) {
//                return false;
//            }
//
//            float connectionLengthSqr = LengthSquared(state.position - surface.position);
//            float absDotNL = Math::Absf(Dot(surface.perturbedNormal, surface.view));
//
//            // -- Update accumulated MIS parameters with info from our new hit position
//            if(state.pathLength > 1 || state.isAreaMeasure) {
//                state.dVCM *= connectionLengthSqr;
//            }
//            state.dVCM *= (1.0f / absDotNL);
//            state.dVC *= (1.0f / absDotNL);
//            state.dVM *= (1.0f / absDotNL);
//
//            // -- store the vertex for use with vertex merging
//            VCMVertex vcmVertex;
//            vcmVertex.throughput = state.throughput;
//            vcmVertex.pathLength = state.pathLength;
//            vcmVertex.index = state.index;
//            vcmVertex.dVCM = state.dVCM;
//            vcmVertex.dVC = state.dVC;
//            vcmVertex.dVM = state.dVM;
//            vcmVertex.hit = hit;
//
//            lightPathSet->lightVertices.Add(vcmVertex);
//
//            // -- connect the path to the camera
//            ConnectLightPathToCamera(context, state, surface, constants.vmWeight, (float)constants.vmCount);
//
//            // -- bsdf scattering to advance the path
//            if(SampleBsdfScattering(&context->sampler, surface, constants.vmWeight, constants.vcWeight, state) == false) {
//                return false;
//            }
//
//            return true;
//        }
//
//        //=========================================================================================================================
//        static void GenerateLightSamples(const VCMIterationConstants& constants, GIIntegratorContext* context,
//                                         uint start, uint end, PathState* results)
//        {
//            for(uint scan = start; scan < end; ++scan) {
//                VCMCommon::GenerateLightSample(context, constants.vcWeight, scan, results[scan]);
//            }
//        }
//
//        //=========================================================================================================================
//        static void EvaluateLightPaths(const VCMIterationConstants& constants, GIIntegratorContext* context,
//                                       uint start, uint end, PathState* pathVertices, LightPathSet* lightPathSet)
//        {
//            // -- generate light paths
//            for(uint scan = start; scan < end; ++scan) {
//
//                // -- create initial light path vertex y_0 
//                PathState& state = pathVertices[scan];
//
//                while(state.pathLength + 2 < context->maxPathLength) {
//
//                    // -- Make a basic ray. No differentials are used atm.
//                    Ray ray = MakeRay(state.position, state.direction);
//
//                    // -- Cast the ray against the scene
//                    HitParameters hit;
//                    if(RayPick(context->rtcScene, ray, hit) == false) {
//                        break;
//                    }
//
//                    // -- Calculate all surface information for this hit position
//                    SurfaceParameters surface;
//                    if(CalculateSurfaceParams(context, &hit, surface) == false) {
//                        break;
//                    }
//
//                    float connectionLengthSqr = LengthSquared(state.position - surface.position);
//                    float absDotNL = Math::Absf(Dot(surface.perturbedNormal, surface.view));
//
//                    // -- Update accumulated MIS parameters with info from our new hit position
//                    if(state.pathLength > 1 || state.isAreaMeasure) {
//                        state.dVCM *= connectionLengthSqr;
//                    }
//                    state.dVCM *= (1.0f / absDotNL);
//                    state.dVC *= (1.0f / absDotNL);
//                    state.dVM *= (1.0f / absDotNL);
//
//                    // -- store the vertex for use with vertex merging
//                    VCMVertex vcmVertex;
//                    vcmVertex.throughput = state.throughput;
//                    vcmVertex.pathLength = state.pathLength;
//                    vcmVertex.index = state.index;
//                    vcmVertex.dVCM = state.dVCM;
//                    vcmVertex.dVC = state.dVC;
//                    vcmVertex.dVM = state.dVM;
//                    vcmVertex.hit = hit;
//                    lightPathSet->lightVertices.Add(vcmVertex);
//
//                    // -- connect the path to the camera
//                    ConnectLightPathToCamera(context, state, surface, constants.vmWeight, (float)constants.vmCount);
//
//                    // -- bsdf scattering to advance the path
//                    if(SampleBsdfScattering(&context->sampler, surface, constants.vmWeight, constants.vcWeight, state) == false) {
//                        break;
//                    }
//                }
//
//                lightPathSet->pathEnds.Add(lightPathSet->lightVertices.Count());
//            }
//        }
//
//        //=========================================================================================================================
//        static void GenerateCameraPaths(const VCMIterationConstants& constants, GIIntegratorContext* context,
//                                        uint start, uint end, PathState* results)
//        {
//            for(uint scan = start; scan < end; ++scan) {
//                uint y = scan / context->camera->width;
//                uint x = scan - y * context->camera->height;
//
//                VCMCommon::GenerateCameraSample(context, x, y, (float)constants.vmCount, results[scan]);
//            }
//        }
//
//        //=========================================================================================================================
//        static void EvaluateCameraPaths(const VCMIterationConstants& constants, const LightPathSet* lightPathSet,
//                                        GIIntegratorContext* context, uint start, uint end, PathState* pathVertices)
//        {
//            for(uint scan = start; scan < end; ++scan) {
//
//                PathState& cameraPathState = pathVertices[scan];
//
//                float3 color = float3::Zero_;
//
//                while(cameraPathState.pathLength < context->maxPathLength) {
//
//                    // -- Make a basic ray. No differentials are used atm...
//                    Ray ray = MakeRay(cameraPathState.position, cameraPathState.direction);
//
//                    // -- Cast the ray against the scene
//                    HitParameters hit;
//                    if(RayPick(context->rtcScene, ray, hit) == false) {
//                        // -- if the ray exits the scene then we sample the ibl and accumulate the results.
//                        float3 sample = cameraPathState.throughput * ConnectToSkyLight(context, cameraPathState);
//                        color += sample;
//                        break;
//                    }
//
//                    // -- Calculate all surface information for this hit position
//                    SurfaceParameters surface;
//                    if(CalculateSurfaceParams(context, &hit, surface) == false) {
//                        break;
//                    }
//
//                    float connectionLengthSqr = LengthSquared(cameraPathState.position - surface.position);
//                    float absDotNL = Math::Absf(Dot(GeometricNormal(surface), surface.view));
//
//                    // -- Update accumulated MIS parameters with info from our new hit position. This combines with work done
//                    // -- at the previous vertex to convert the solid angle pdf to the area pdf of the outermost term.
//                    cameraPathState.dVCM *= connectionLengthSqr;
//                    cameraPathState.dVCM /= absDotNL;
//                    cameraPathState.dVC /= absDotNL;
//                    cameraPathState.dVM /= absDotNL;
//
//                    // -- Vertex connection to a light source
//                    if(cameraPathState.pathLength + 1 < context->maxPathLength) {
//                        float3 sample = cameraPathState.throughput * ConnectCameraPathToLight(context, cameraPathState, surface,
//                                                                                              constants.vmWeight);
//                        color += sample;
//                    }
//
//                    // -- Vertex connection to a light vertex
//                    {
//                        for(uint vcScan = 0; vcScan < constants.vcCount; ++vcScan) {
//
//                            uint vcIndex = constants.vcCount * cameraPathState.index + vcScan;
//                            uint pathStart = (vcIndex == 0) ? 0 : lightPathSet->pathEnds[vcIndex - 1];
//                            uint pathEnd = lightPathSet->pathEnds[vcIndex];
//
//                            for(uint lightVertexIndex = pathStart; lightVertexIndex < pathEnd; ++lightVertexIndex) {
//                                const VCMVertex& lightVertex = lightPathSet->lightVertices[lightVertexIndex];
//                                if(lightVertex.pathLength + 1 + cameraPathState.pathLength > context->maxPathLength) {
//                                    break;
//                                }
//
//                                color += cameraPathState.throughput * lightVertex.throughput
//                                      * ConnectPathVertices(context, surface, cameraPathState, lightVertex, constants.vmWeight);
//                            }
//                        }
//                    }
//
//                    // -- Vertex merging
//                    {
//                        VertexMergingCallbackStruct callbackData;
//                        callbackData.context = context;
//                        callbackData.surface = &surface;
//                        callbackData.pathVertices = &lightPathSet->lightVertices;
//                        callbackData.cameraState = &cameraPathState;
//                        callbackData.vcWeight = constants.vcWeight;
//                        callbackData.result = float3::Zero_;
//                        SearchHashGrid(&lightPathSet->hashGrid, lightPathSet->lightVertices, surface.position, &callbackData,
//                                       MergeVertices);
//
//                        color += cameraPathState.throughput * constants.vmNormalization * callbackData.result;
//                    }
//
//                    // -- bsdf scattering to advance the path
//                    if(SampleBsdfScattering(&context->sampler, surface, constants.vmWeight, constants.vcWeight,
//                                            cameraPathState) == false) {
//                        break;
//                    }
//                }
//
//                FramebufferWriter_Write(&context->frameWriter, &color, 1, cameraPathState.index);
//            }
//        }
//
//        //=========================================================================================================================
//        static void VertexConnectionAndMerging(CArray<PathState>& pathVertices, GIIntegratorContext* context,
//                                               LightPathSet* lightPathSet, const VCMIterationConstants& constants,
//                                               uint width, uint height)
//        {
//            ProfileEventMarker_(0, "VertexConnectionAndMerging");
//
//            pathVertices.Clear();
//            pathVertices.Resize((uint32)constants.vmCount);
//
//            lightPathSet->lightVertices.Clear();
//            lightPathSet->pathEnds.Clear();
//            lightPathSet->lightVertices.Reserve((uint32)(constants.vmCount));
//            lightPathSet->pathEnds.Reserve((uint32)(constants.vmCount));
//
//            {
//                GenerateLightSamples(constants, context, 0, constants.vmCount, pathVertices.DataPointer());
//                EvaluateLightPaths(constants, context, 0, constants.vmCount, pathVertices.DataPointer(), lightPathSet);
//            }
//
//            // -- build the hash grid
//            BuildHashGrid(&lightPathSet->hashGrid, constants.vmCount, constants.vmSearchRadius, lightPathSet->lightVertices);
//
//            {
//                GenerateCameraPaths(constants, context, 0, constants.vmCount, pathVertices.DataPointer());
//                EvaluateCameraPaths(constants, lightPathSet, context, 0, constants.vmCount, pathVertices.DataPointer());
//            }
//        }
//
//        //=========================================================================================================================
//        static void VCMKernel(void* userData)
//        {
//            VCMSharedData* sharedData = static_cast<VCMSharedData*>(userData);
//            int64 kernelIndex = Atomic::Increment64(sharedData->kernelIndices);
//
//            uint width = sharedData->camera.width;
//            uint height = sharedData->camera.height;
//
//            GIIntegratorContext context;
//            context.rtcScene = sharedData->rtcScene;
//            context.scene = sharedData->scene;
//            context.camera = &sharedData->camera;
//            context.sampler.Initialize((uint32)kernelIndex);
//            context.maxPathLength = sharedData->maxBounceCount;
//            FramebufferWriter_Initialize(&context.frameWriter, sharedData->frame,
//                                         DefaultFrameWriterCapacity_, DefaultFrameWriterSoftCapacity_);
//
//            LightPathSet lightPathSet;
//
//            CArray<PathState> pathVertices;
//
//            int64 iterationCount = 0;
//            float elapsedSeconds = 0.0f;
//            while(elapsedSeconds < sharedData->integrationSeconds) {
//                int64 index = Atomic::Increment64(sharedData->vcmPassCount);
//                float iterationIndex = index + 1.0f;
//
//                uint vmCount = 1 * width * height;
//                uint vcCount = 1;
//
//                VCMIterationConstants constants = VCMCommon::CalculateIterationConstants(vmCount, vcCount, sharedData->vcmRadius,
//                                                                                         sharedData->vcmRadiusAlpha,
//                                                                                         iterationIndex);
//
//                VertexConnectionAndMerging(pathVertices, &context, &lightPathSet, constants, width, height);
//                ++iterationCount;
//
//                elapsedSeconds = SystemTime::ElapsedSecondsF(sharedData->integrationStartTime);
//            }
//
//            ShutdownHashGrid(&lightPathSet.hashGrid);
//            lightPathSet.lightVertices.Shutdown();
//            lightPathSet.pathEnds.Shutdown();
//
//            Atomic::Add64(sharedData->iterationsPerPixel, iterationCount);
//
//            context.sampler.Shutdown();
//
//            FramebufferWriter_Shutdown(&context.frameWriter);
//            Atomic::Increment64(sharedData->completedThreads);
//        }
//
//        //=========================================================================================================================
//        void GenerateImage(SceneResource* scene, const RayCastCameraSettings& camera, cpointer imageName)
//        {
//            Framebuffer frame;
//            FrameBuffer_Initialize(&frame, (uint32)camera.viewportWidth, (uint32)camera.viewportHeight, 1);
//
//            int64 completedThreads = 0;
//            int64 kernelIndex = 0;
//            int64 iterationsPerPixel = 0;
//            int64 vcmPassCount = 0;
//
//            #if EnableMultiThreading_ 
//                const uint additionalThreadCount = 6;
//            #else
//                const uint additionalThreadCount = 0;
//            #endif
//
//            VCMSharedData sharedData;
//            sharedData.scene                  = scene;
//            sharedData.camera                 = camera;
//            sharedData.frame                  = &frame;
//            sharedData.maxBounceCount         = MaxBounceCount_;
//            sharedData.integrationStartTime   = SystemTime::Now();
//            sharedData.integrationSeconds     = IntegrationSeconds_;
//            sharedData.iterationsPerPixel     = &iterationsPerPixel;
//            sharedData.vcmPassCount           = &vcmPassCount;
//            sharedData.completedThreads       = &completedThreads;
//            sharedData.kernelIndices          = &kernelIndex;
//            sharedData.vcmRadius              = VcmRadiusFactor_ * scene->boundingSphere.w;
//            sharedData.vcmRadiusAlpha         = VcmRadiusAlpha_;
//
//            #if EnableMultiThreading_
//                ThreadHandle threadHandles[additionalThreadCount];
//
//                // -- fork threads
//                for(uint scan = 0; scan < additionalThreadCount; ++scan) {
//                    threadHandles[scan] = CreateThread(VCMKernel, &sharedData);
//                }
//            #endif
//
//            // -- do work on the main thread too
//            VCMKernel(&sharedData);
//
//            #if EnableMultiThreading_ 
//                // -- wait for any other threads to finish
//                while(*sharedData.completedThreads != *sharedData.kernelIndices);
//
//                for(uint scan = 0; scan < additionalThreadCount; ++scan) {
//                    ShutdownThread(threadHandles[scan]);
//                }
//            #endif
//
//            Logging::WriteDebugInfo("Vcm integration performed with %lld iterations", iterationsPerPixel);
//
//            FrameBuffer_Normalize(&frame, (1.0f / iterationsPerPixel));
//
//            FrameBuffer_Save(&frame, imageName);
//            FrameBuffer_Shutdown(&frame);
//        }
//    }
//}
