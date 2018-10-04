
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

#define MaxBounceCount_         2048

#define AdditionalThreadCount_  6
#define PathsPerPixel_          16
#define LayerCount_             2

namespace Selas
{
    namespace DeferredPathTracer
    {
        struct DeferredRay
        {
            float3 origin;
            float3 direction; // JSTODO - Octrahedral
            float3 throughput; // JSTODO - RGB9e5 or half
            uint32 xy;
            float  error;
        };

        // -- Primary ray generation
        // -- Enqueuing rays into the 6 cardinal direction batches
        // -- Sorting ray batches
        // -- Bulk ray tracing


        //=========================================================================================================================
        void GenerateImage(GeometryCache* geometryCache, TextureCache* textureCache, SceneResource* scene,
                           const RayCastCameraSettings& camera, cpointer imageName)
        {
            Framebuffer frame;
            FrameBuffer_Initialize(&frame, (uint32)camera.viewportWidth, (uint32)camera.viewportHeight, LayerCount_);

            int64 completedThreads = 0;
            int64 kernelIndex = 0;
            uint64 pixelIndex = 0;

            //PathTracingKernelData integratorContext;
            //integratorContext.geometryCache          = geometryCache;
            //integratorContext.textureCache           = textureCache;
            //integratorContext.scene                  = scene;
            //integratorContext.camera                 = camera;
            //integratorContext.maxBounceCount         = MaxBounceCount_;
            //integratorContext.pathsPerPixel          = PathsPerPixel_;
            //integratorContext.integrationStartTime   = SystemTime::Now();
            //integratorContext.pixelIndex             = &pixelIndex;
            //integratorContext.completedThreads       = &completedThreads;
            //integratorContext.kernelIndices          = &kernelIndex;
            //integratorContext.frame                  = &frame;

            //#if AdditionalThreadCount_ > 0
            //    ThreadHandle threadHandles[AdditionalThreadCount_];

            //    // -- fork threads
            //    for(uint scan = 0; scan < AdditionalThreadCount_; ++scan) {
            //        threadHandles[scan] = CreateThread(PathTracerKernel, &integratorContext);
            //    }
            //#endif

            //// -- do work on the main thread too
            //PathTracerKernel(&integratorContext);

            //#if AdditionalThreadCount_ > 0
            //    // -- wait for any other threads to finish
            //    while(*integratorContext.completedThreads != *integratorContext.kernelIndices);

            //    for(uint scan = 0; scan < AdditionalThreadCount_; ++scan) {
            //        ShutdownThread(threadHandles[scan]);
            //    }
            //#endif

            FrameBuffer_Normalize(&frame, (1.0f / PathsPerPixel_));

            FrameBuffer_Save(&frame, imageName);
            FrameBuffer_Shutdown(&frame);
        }
    }
}
