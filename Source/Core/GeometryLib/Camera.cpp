//==============================================================================
// Joe Schutte
//==============================================================================

#include "GeometryLib/Camera.h"
#include "GeometryLib/Ray.h"
#include "MathLib/FloatFuncs.h"
#include "MathLib/Random.h"

namespace Selas
{
    //==============================================================================
    int2 WorldToImage(const RayCastCameraSettings* __restrict camera, float3 world)
    {
        float4 clip = MatrixMultiplyFloat4(float4(world, 1.0f), camera->worldToClip);

        int2 image;
        image.x = (int32)Math::Floor(( clip.x / clip.w * 0.5f + 0.5f) * camera->viewportWidth  + 0.5f);
        image.y = (int32)Math::Floor((-clip.y / clip.w * 0.5f + 0.5f) * camera->viewportHeight + 0.5f);
        return image;
    }

    //==============================================================================
    float3 ImageToWorld(const RayCastCameraSettings* __restrict camera, float x, float y)
    {
        float4 un = MatrixMultiplyFloat4(float4(x, y, 0.0f, 1.0f), camera->imageToWorld);
        return (1.0f / un.w) * float3(un.x, un.y, un.z);
    }

    //==============================================================================
    Ray JitteredCameraRay(const RayCastCameraSettings* __restrict camera, Random::MersenneTwister* twister, float viewX, float viewY)
    {
        // JSTODO - Blue noise probably ideal here. Maybe this: http://liris.cnrs.fr/david.coeurjolly/publications/perrier18eg.html ?
        float vx = viewX + MersenneTwisterFloat(twister);
        float vy = viewY + MersenneTwisterFloat(twister);

        float3 p  = ImageToWorld(camera, vx, vy);
        float3 px = ImageToWorld(camera, vx + 1.0f, vy);
        float3 py = ImageToWorld(camera, vx, vy + 1.0f);

        float3 d  = Normalize(p  - camera->position);
        float3 dx = Normalize(px - camera->position);
        float3 dy = Normalize(py - camera->position);

        Ray result;
        result.origin      = p;
        result.direction   = d;
        result.rxOrigin    = px;
        result.rxDirection = dx;
        result.ryOrigin    = py;
        result.ryDirection = dy;

        return result;
    }

    //==============================================================================
    void InitializeRayCastCamera(const CameraSettings& settings, uint width, uint height, RayCastCameraSettings& camera)
    {
        float aspect = (float)width / height;
        float horizontalFov = settings.fov;
        float verticalFov = 2.0f * Math::Atanf(horizontalFov * 0.5f) * aspect;
        float widthf = (float)width;
        float heightf = (float)height;

        float4x4 worldToView = LookAtLh(settings.position, settings.up, settings.lookAt);
        float4x4 viewToClip  = PerspectiveFovLhProjection(verticalFov, aspect, settings.znear, settings.zfar);

        float4x4 worldToClip   = MatrixMultiply(worldToView, viewToClip);
        float4x4 clipToWorld = MatrixInverse(worldToClip);

        float4x4 imageToClip = Matrix4x4::ScaleTranslate(2.0f / widthf, -2.0f / heightf, 0, -1, 1, 0);
        float4x4 clipToImage = MatrixMultiply(Matrix4x4::ScaleTranslate(0.5f, -0.5f, 0, 1, -1, 0), Matrix4x4::Scale(widthf, heightf, 1.0f));
        
        camera.imageToWorld              = MatrixMultiply(imageToClip, clipToWorld);
        camera.worldToClip               = worldToClip;
        camera.forward                   = Normalize(settings.lookAt - settings.position);
        camera.viewportWidth             = widthf;
        camera.viewportHeight            = heightf;
        camera.position                  = settings.position;
        camera.znear                     = settings.znear;
        camera.zfar                      = settings.zfar;
        camera.virtualImagePlaneDistance = widthf / (2.0f * Math::Tanf(horizontalFov));
    }
}