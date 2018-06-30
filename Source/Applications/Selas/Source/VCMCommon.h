#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "Shading/SurfaceParameters.h"
#include "Shading/IntegratorContexts.h"
#include "UtilityLib/Color.h"
#include "MathLib/FloatStructs.h"
#include "ContainersLib/CArray.h"
#include "SystemLib/BasicTypes.h"

#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>

namespace Selas
{
    struct GIIntegrationContext;
    struct SceneContext;
    struct Framebuffer;

    // JSTODO - This uses a lot of memory. Maybe store hit position and resample the material for each use?
    struct VcmVertex
    {
        float3 throughput;
        uint32 pathLength;
        float dVCM;
        float dVC;
        float dVM;

        SurfaceParameters surface;
    };

    struct VertexMergingCallbackStruct
    {
        const GIIntegrationContext* context;
        const SurfaceParameters* surface;
        const CArray<VcmVertex>* pathVertices;
        const PathState* cameraState;
        float vcWeight;

        float3 result;
    };

    namespace VCMCommon
    {
        void GenerateLightSample(GIIntegrationContext* context, float vcWeight, PathState& state);
        void GenerateCameraSample(GIIntegrationContext* context, uint x, uint y, float lightPathCount, PathState& state);

        float SearchRadius(float baseRadius, float radiusAlpha, float iterationIndex);
    }
}