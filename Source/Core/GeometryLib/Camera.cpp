//==============================================================================
// Joe Schutte
//==============================================================================

#include <GeometryLib/Camera.h>
#include <GeometryLib/Ray.h>
#include <MathLib/FloatFuncs.h>
#include <MathLib/Random.h>

namespace Shooty
{
    //==============================================================================
    static float3 ScreenToWorld(const RayCastCameraSettings* __restrict camera, float x, float y)
    {
        x = (2.0f * x) / camera->viewportWidth - 1.0f;
        y = 1.0f - (2.0f * y) / camera->viewportHeight;

        float4 un = MatrixMultiplyFloat4(float4(x, y, 0.0f, 1.0f), camera->invViewProjection);
        return (1.0f / un.w) * float3(un.x, un.y, un.z);
    }

    //==============================================================================
    Ray JitteredCameraRay(const RayCastCameraSettings* __restrict camera, Random::MersenneTwister* twister, uint32 pixelIndex, float viewX, float viewY)
    {
        // JSTODO - Blue noise probably ideal here. Maybe this: http://liris.cnrs.fr/david.coeurjolly/publications/perrier18eg.html ?
        float vx = viewX + MersenneTwisterFloat(twister);
        float vy = viewY + MersenneTwisterFloat(twister);

        float3 p  = ScreenToWorld(camera, vx, vy);
        float3 px = ScreenToWorld(camera, vx + 1.0f, vy);
        float3 py = ScreenToWorld(camera, vx, vy + 1.0f);

        float3 d  = Normalize(p  - camera->position);
        float3 dx = Normalize(px - camera->position);
        float3 dy = Normalize(py - camera->position);

        // JSTODO - Should the origins be set to p, px, and py rather than camera->position for all 3?
        Ray result;
        result.origin    = camera->position;
        result.direction = d;
        result.throughput = float3::One_;
        result.pixelIndex = pixelIndex;

        result.rxOrigin    = camera->position;
        result.rxDirection = dx;
        result.ryOrigin    = camera->position;
        result.ryDirection = dy;

        result.bounceCount = 0;

        return result;
    }
}