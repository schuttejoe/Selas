#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "GeometryLib/Ray.h"
#include "MathLib/FloatStructs.h"
#include "MathLib/IntStructs.h"

namespace Selas
{
    namespace Random
    {
        struct MersenneTwister;
    }

    struct CameraSettings
    {
        float3 position;
        float  fov;
        float3 lookAt;
        float  znear;
        float3 up;
        float  zfar;
    };

    //==============================================================================
    struct RayCastCameraSettings
    {
        float4x4 imageToWorld;
        float4x4 worldToClip;
        float3   position;
        float3   forward;
        float    viewportWidth;
        float    viewportHeight;
        float    znear;
        float    zfar;

        // -- the distance from the camera that you'd have to travel before the area of a single pixel is 1.
        float    virtualImagePlaneDistance;
    };

    int2 WorldToImage(const RayCastCameraSettings* __restrict camera, float3 world);
    float3 ImageToWorld(const RayCastCameraSettings* __restrict camera, float x, float y);

    Ray JitteredCameraRay(const RayCastCameraSettings* __restrict camera, Random::MersenneTwister* twister, float viewX, float viewY);

    void InitializeRayCastCamera(const CameraSettings& settings, uint width, uint height, RayCastCameraSettings& camera);
}