//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/Frustum.h>
#include <MathLib/FloatFuncs.h>
#include <MathLib/FloatStructs.h>

namespace Shooty
{
    namespace Math
    {
        //==============================================================================
        static float4 NormalizePlane(float4 plane)
        {
            float ooLength = 1.0f / Math::Sqrtf(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
            return ooLength * plane;
        }

        //==============================================================================
        void CalculateFrustumPlanes(const float4x4& projection, float4* planes)
        {
            float4 top_plane;
            top_plane.x = projection.r0.w - projection.r0.y;
            top_plane.y = projection.r1.w - projection.r1.y;
            top_plane.z = projection.r2.w - projection.r2.y;
            top_plane.w = projection.r3.w - projection.r3.y;
            planes[0] = NormalizePlane(top_plane);

            float4 bottom_plane;
            bottom_plane.x = projection.r0.w + projection.r0.y;
            bottom_plane.y = projection.r1.w + projection.r1.y;
            bottom_plane.z = projection.r2.w + projection.r2.y;
            bottom_plane.w = projection.r3.w + projection.r3.y;
            planes[1] = NormalizePlane(bottom_plane);

            float4 left_plane;
            left_plane.x = projection.r0.w + projection.r0.x;
            left_plane.y = projection.r1.w + projection.r1.x;
            left_plane.z = projection.r2.w + projection.r2.x;
            left_plane.w = projection.r3.w + projection.r3.x;
            planes[2] = NormalizePlane(left_plane);

            float4 right_plane;
            right_plane.x = projection.r0.w - projection.r0.x;
            right_plane.y = projection.r1.w - projection.r1.x;
            right_plane.z = projection.r2.w - projection.r2.x;
            right_plane.w = projection.r3.w - projection.r3.x;
            planes[3] = NormalizePlane(right_plane);

            float4 near_plane;
            near_plane.x = projection.r0.z;
            near_plane.y = projection.r1.z;
            near_plane.z = projection.r2.z;
            near_plane.w = projection.r3.z;
            planes[4] = NormalizePlane(near_plane);

            float4 far_plane;
            far_plane.x = projection.r0.w - projection.r0.z;
            far_plane.y = projection.r1.w - projection.r1.z;
            far_plane.z = projection.r2.w - projection.r2.z;
            far_plane.w = projection.r3.w - projection.r3.z;
            planes[5] = NormalizePlane(far_plane);
        }
    }
}