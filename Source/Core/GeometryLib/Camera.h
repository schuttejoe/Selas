#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <GeometryLib/Ray.h>
#include <MathLib/FloatStructs.h>

namespace Shooty
{
    namespace Random
    {
        struct MersenneTwister;
    }

    //==============================================================================
    struct RayCastCameraSettings
    {
        float4x4 invViewProjection;
        float3   position;
        float    viewportWidth;
        float    viewportHeight;
        float    znear;
        float    zfar;
    };

    Ray JitteredCameraRay(const RayCastCameraSettings* __restrict camera, Random::MersenneTwister* twister, uint32 pixelIndex, float viewX, float viewY);
}