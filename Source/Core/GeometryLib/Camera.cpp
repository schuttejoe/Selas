//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "GeometryLib/Camera.h"
#include "GeometryLib/Ray.h"
#include "IoLib/Serializer.h"
#include "MathLib/FloatFuncs.h"
#include "MathLib/Sampler.h"

namespace Selas
{
    //=============================================================================================================================
    void Serialize(CSerializer* serializer, CameraSettings& data)
    {
        Serialize(serializer, data.position);
        Serialize(serializer, data.fov);
        Serialize(serializer, data.lookAt);
        Serialize(serializer, data.znear);
        Serialize(serializer, data.up);
        Serialize(serializer, data.zfar);
    }

    //=============================================================================================================================
    void InvalidCameraSettings(CameraSettings* settings)
    {
        settings->position = float3(0.0f, 0.0f, 0.0f);
        settings->lookAt   = float3(0.0f, 0.0f, 0.0f);
        settings->up       = float3(0.0f, 0.0f, 0.0f);
        settings->fov      = 0.0f * Math::DegreesToRadians_;
        settings->znear    = 0.0f;
        settings->zfar     = 0.0f;
    }

    //=============================================================================================================================
    void DefaultCameraSettings(CameraSettings* settings)
    {
        settings->position = float3(-1139.01589265f, 23.28673313185658f, 1479.7947229f);
        settings->lookAt   = float3(244.81433650665076f, 238.8071478842799f, 560.3801168449178f);
        settings->up       = float3(-0.107f, 0.99169f, 0.071189f);
        settings->fov      = 70.0f * Math::DegreesToRadians_;
        settings->znear    = 0.1f;
        settings->zfar     = 50000.0f;
    }

    //=============================================================================================================================
    bool ValidCamera(const CameraSettings& settings)
    {
        return settings.fov > 0 && settings.zfar > settings.znear;
    }

    //=============================================================================================================================
    int2 WorldToImage(const RayCastCameraSettings* __restrict camera, float3 world)
    {
        float4 clip = MatrixMultiplyFloat4(float4(world, 1.0f), camera->worldToClip);

        int2 image;
        image.x = (int32)Math::Floor(( clip.x / clip.w * 0.5f + 0.5f) * camera->viewportWidth  + 0.5f);
        image.y = (int32)Math::Floor((-clip.y / clip.w * 0.5f + 0.5f) * camera->viewportHeight + 0.5f);
        return image;
    }

    //=============================================================================================================================
    float3 ImageToWorld(const RayCastCameraSettings* __restrict camera, float x, float y)
    {
        float4 un = MatrixMultiplyFloat4(float4(x, y, 0.0f, 1.0f), camera->imageToWorld);
        return (1.0f / un.w) * float3(un.x, un.y, un.z);
    }

    //=============================================================================================================================
    Ray JitteredCameraRay(const RayCastCameraSettings* __restrict camera, CSampler* sampler, float viewX, float viewY)
    {
        // JSTODO - Blue noise probably ideal here. Maybe this: http://liris.cnrs.fr/david.coeurjolly/publications/perrier18eg.html ?
        float vx = viewX + sampler->UniformFloat();
        float vy = viewY + sampler->UniformFloat();

        float3 p  = ImageToWorld(camera, vx, vy);
        float3 d  = Normalize(p  - camera->position);

        Ray result;
        result.origin      = p;
        result.direction   = d;

        return result;
    }

    //=============================================================================================================================
    void InitializeRayCastCamera(const CameraSettings& settings, uint width, uint height, RayCastCameraSettings& camera)
    {
        float widthf        = (float)width;
        float heightf       = (float)height;
        float aspect        = widthf / heightf;
        float horizontalFov = settings.fov;

        float4x4 worldToView = LookAtRh(settings.position, settings.up, settings.lookAt);
        float4x4 viewToClip  = PerspectiveFovRhProjection(horizontalFov, aspect, settings.znear, settings.zfar);

        float4x4 viewToWorld = MatrixInverse(worldToView);
        float4x4 clipToView  = MatrixInverse(viewToClip);

        float4x4 worldToClip = MatrixMultiply(worldToView, viewToClip);
        float4x4 clipToWorld = MatrixMultiply(clipToView, viewToWorld);

        float4x4 imageToClip = Matrix4x4::ScaleTranslate(2.0f / widthf, -2.0f / heightf, 0, -1, 1, 0);
        
        camera.imageToWorld              = MatrixMultiply(imageToClip, clipToWorld);
        camera.worldToClip               = worldToClip;
        camera.forward                   = Normalize(settings.lookAt - settings.position);
        camera.viewportWidth             = widthf;
        camera.viewportHeight            = heightf;
        camera.position                  = settings.position;
        camera.znear                     = settings.znear;
        camera.zfar                      = settings.zfar;
        camera.virtualImagePlaneDistance = widthf / (2.0f * Math::Tanf(horizontalFov));
        camera.width                     = width;
        camera.height                    = height;
    }
}