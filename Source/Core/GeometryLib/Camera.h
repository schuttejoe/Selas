#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <GeometryLib/Ray.h>
#include <MathLib/FloatStructs.h>
#include <MathLib/IntStructs.h>

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
        float    imagePlaneDistance;
    };

    int2 WorldToImage(const RayCastCameraSettings* __restrict camera, float3 world);
    float3 ImageToWorld(const RayCastCameraSettings* __restrict camera, float x, float y);

    Ray JitteredCameraRay(const RayCastCameraSettings* __restrict camera, Random::MersenneTwister* twister, uint32 pixelIndex, float viewX, float viewY);

    void InitializeRayCastCamera(const CameraSettings& settings, uint width, uint height, RayCastCameraSettings& camera);
}