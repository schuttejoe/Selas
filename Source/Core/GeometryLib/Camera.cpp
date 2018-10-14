//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "GeometryLib/Camera.h"
#include "GeometryLib/Ray.h"
#include "IoLib/Serializer.h"
#include "ContainersLib/Rect.h"
#include "MathLib/CorrelatedMultiJitter.h"
#include "MathLib/FloatFuncs.h"
#include "MathLib/Sampler.h"

namespace Selas
{
    //=============================================================================================================================
    void Serialize(CSerializer* serializer, CameraSettings& data)
    {
        Serialize(serializer, data.name);
        Serialize(serializer, data.position);
        Serialize(serializer, data.fovDegrees);
        Serialize(serializer, data.lookAt);
        Serialize(serializer, data.znear);
        Serialize(serializer, data.up);
        Serialize(serializer, data.zfar);
    }

    //=============================================================================================================================
    void InvalidCameraSettings(CameraSettings* settings)
    {
        settings->position   = float3(0.0f, 0.0f, 0.0f);
        settings->lookAt     = float3(0.0f, 0.0f, 0.0f);
        settings->up         = float3(0.0f, 0.0f, 0.0f);
        settings->fovDegrees = 0.0f;
        settings->znear      = 0.0f;
        settings->zfar       = 0.0f;
    }

    //=============================================================================================================================
    void DefaultCameraSettings(CameraSettings* settings)
    {
        settings->position   = float3(0.0f, 0.0f, 5.0f);
        settings->lookAt     = float3(0.0f, 0.0f, 0.0f);
        settings->up         = float3(0.0f, 1.0f, 0.0f);
        settings->fovDegrees = 35.0f;
        settings->znear      = 0.01f;
        settings->zfar       = 10000.0f;
    }

    //=============================================================================================================================
    bool ValidCamera(const CameraSettings& settings)
    {
        return settings.fovDegrees > 0 && settings.zfar > settings.znear;
    }

    //=============================================================================================================================
    static float3 ImageToWorldDirection(const RayCastCameraSettings* __restrict camera, float x, float y)
    {
        float2 clip = float2(((x / camera->width) * 2.0f - 1.0f), 1.0f - 2.0f * y / camera->height);
        return Normalize(camera->cameraZ - clip.x * camera->cameraX + clip.y * camera->cameraY);
    }

    //=============================================================================================================================
    Ray JitteredCameraRay(const RayCastCameraSettings* __restrict camera, CSampler* sampler, float viewX, float viewY)
    {
        // JSTODO - Blue noise probably ideal here. Maybe this: http://liris.cnrs.fr/david.coeurjolly/publications/perrier18eg.html ?
        float vx = viewX + sampler->UniformFloat();
        float vy = viewY + sampler->UniformFloat();

        Ray result;
        result.origin      = camera->position;
        result.direction   = ImageToWorldDirection(camera, vx, vy);

        return result;
    }

    //=============================================================================================================================
    Ray JitteredCameraRay(const RayCastCameraSettings* __restrict camera, int32 x, int32 y, int32 s, int32 m, int32 n, int32 p)
    {
        float2 v = float2((float)x, (float)y) + CorrelatedMultiJitter(s, m, n, p);

        Ray result;
        result.origin    = camera->position;
        result.direction = ImageToWorldDirection(camera, v.x, v.y);

        return result;
    }

    //=============================================================================================================================
    void InitializeRayCastCamera(const CameraSettings& settings, uint width, uint height, RayCastCameraSettings& camera)
    {
        float widthf        = (float)width;
        float heightf       = (float)height;
        float aspect        = widthf / heightf;
        float horizontalFov = settings.fovDegrees * Math::DegreesToRadians_;
        float hLength       = Math::Tanf(horizontalFov * 0.5f);
        float verticalFov   = 2 * Math::Atanf(hLength / aspect);
        float vLength       = Math::Tanf(verticalFov * 0.5f);
       
        float3 cameraForward = Normalize(settings.lookAt - settings.position);
        float3 cameraRight = Normalize(Cross(settings.up, cameraForward));
        
        camera.cameraX = hLength * cameraRight;
        camera.cameraY = vLength * Cross(cameraForward, cameraRight);
        camera.cameraZ = cameraForward;

        camera.forward                   = Normalize(settings.lookAt - settings.position);
        camera.viewportWidth             = widthf;
        camera.viewportHeight            = heightf;
        camera.position                  = settings.position;
        camera.znear                     = settings.znear;
        camera.zfar                      = settings.zfar;
        camera.virtualImagePlaneDistance = widthf / (2.0f * Math::Tanf(horizontalFov));
        camera.width                     = width;
        camera.height                    = height;
        camera.aspect                    = aspect;
    }
}