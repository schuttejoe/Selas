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

#define PathStateIndexBitCount_         26
#define PathStatePathLengthBitCount_    5
#define PathStateIsAreaMeasureBitCount_ 1

namespace Selas
{
    struct GIIntegrationContext;
    struct SceneContext;
    struct Framebuffer;

    struct VCMIterationConstants
    {
        uint vmCount;
        uint vcCount;
        float vmSearchRadius;
        float vmSearchRadiusSqr;
        float vmNormalization;
        float vmWeight;
        float vcWeight;
    };

    struct VCMVertex
    {
        float3 throughput;
        uint32 index      : PathStateIndexBitCount_;
        uint32 pathLength : PathStatePathLengthBitCount_;
        float dVCM;
        float dVC;
        float dVM;

        HitParameters hit;
    };

    struct PathState
    {
        float3 position;
        float3 direction;  // JSTODO - convert to octrahedral format
        float3 throughput; // JSTODO - Testing convert to RGB9e5? Or even half format?
        float dVCM;        // JSTODO - Test as half format
        float dVC;         // JSTODO - Test as half format
        float dVM;         // JSTODO - Test as half format

        uint32 index         : PathStateIndexBitCount_;         // caps paths per iteration to 67,108,864
        uint32 pathLength    : PathStatePathLengthBitCount_;    // caps max path length to 32
        uint32 isAreaMeasure : PathStateIsAreaMeasureBitCount_;
    };

    struct VertexMergingCallbackStruct
    {
        const GIIntegrationContext* context;
        const SurfaceParameters* surface;
        const CArray<VCMVertex>* pathVertices;
        const PathState* cameraState;
        float vcWeight;

        float3 result;
    };

    namespace VCMCommon
    {
        void GenerateLightSample(GIIntegrationContext* context, float vcWeight, uint index, PathState& state);
        void GenerateCameraSample(GIIntegrationContext* context, uint x, uint y, float lightPathCount, PathState& state);

        float SearchRadius(float baseRadius, float radiusAlpha, float iterationIndex);
        VCMIterationConstants CalculateIterationConstants(uint vmCount, uint vcCount, float baseRadius, float radiusAlpha, float iterationIndex);
    }
}