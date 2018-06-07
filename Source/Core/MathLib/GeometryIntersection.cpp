//==============================================================================
// Joe Schutte
//==============================================================================

#include "MathLib/GeometryIntersection.h"
#include "MathLib/FloatFuncs.h"

#include "MathLib/Trigonometric.h"
#include "SystemLib/JsAssert.h"
#include "SystemLib/MinMax.h"

namespace Selas
{
    namespace Intersection
    {
        //==============================================================================
        void RaySphereNearest(float3 o, float3 d, float3 center, float r, float3& p)
        {
            float3 x = o - center;
            float dotOD = Dot(o, d);
            float dotDX = Dot(d, x);

            float a = 1;
            float b = 2.0f * dotDX;
            float c = Dot(x, x) - r * r;

            float t;

            float root = b * b - 4.0f * a * c;
            if(root < 0.0f) {
                t = Length(ProjectOntoV(x, d));
            }
            else if(root == 0.0f) {
                t = -0.5f * b / a;
            }
            else {
                float q = (b > 0) ? -0.5f * (b + Math::Sqrtf(root)) : -0.5f * (b - Math::Sqrtf(root));
                float t0 = q / a;
                float t1 = c / q;

                t = Min<float>(t0, t1);
            }

            p = o + t * d;
        }

        //==============================================================================
        bool RaySphere(float3 o, float3 d, float3 center, float r, float3& p)
        {
            float3 x = o - center;
            float dotOD = Dot(o, d);
            float dotDX = Dot(d, x);

            float a = 1;
            float b = 2.0f * dotDX;
            float c = Dot(x, x) - r * r;

            float t;

            float root = b * b - 4.0f * a * c;
            if(root < 0.0f) {
                return false;
            }
            else if(root == 0.0f) {
                t = -0.5f * b / a;
            }
            else {
                float q = (b > 0) ? -0.5f * (b + Math::Sqrtf(root)) : -0.5f * (b - Math::Sqrtf(root));
                float t0 = q / a;
                float t1 = c / q;

                t = Min<float>(t0, t1);
            }

            p = o + t * d;
            return true;
        }

        //==============================================================================
        bool RayAABox(float3 origin, float3 direction, float3 minPoint, float3 maxPoint)
        {
            float t0 = -9999999.0f;
            float t1 = 9999999.0f;

            float inverseRayDirectionX = 1.0f / direction.x;
            float tNearX = (minPoint.x - origin.x) * inverseRayDirectionX;
            float tFarX = (maxPoint.x - origin.x) * inverseRayDirectionX;

            if(tNearX > tFarX) {
                float temp = tFarX;
                tFarX = tNearX;
                tNearX = temp;
            }
            t0 = (tNearX > t0) ? tNearX : t0;
            t1 = (tFarX < t1) ? tFarX : t1;
            if(t0 > t1) {
                return false;
            }

            float inverseRayDirectionY = 1.0f / direction.y;
            float tNearY = (minPoint.y - origin.y) * inverseRayDirectionY;
            float tFarY = (maxPoint.y - origin.y) * inverseRayDirectionY;

            if(tNearY > tFarY) {
                float temp = tFarY;
                tFarY = tNearY;
                tNearY = temp;
            }
            t0 = (tNearY > t0) ? tNearY : t0;
            t1 = (tFarY < t1) ? tFarY : t1;
            if(t0 > t1) {
                return false;
            }

            float inverseRayDirectionZ = 1.0f / direction.z;
            float t_near_z = (minPoint.z - origin.z) * inverseRayDirectionZ;
            float t_far_z = (maxPoint.z - origin.z) * inverseRayDirectionZ;

            if(t_near_z > t_far_z) {
                float temp = t_far_z;
                t_far_z = t_near_z;
                t_near_z = temp;
            }
            t0 = (t_near_z > t0) ? t_near_z : t0;
            t1 = (t_far_z < t1) ? t_far_z : t1;
            if(t0 > t1) {
                return false;
            }

            return true;
        }

        //==============================================================================
        bool SweptSphereSphere(float3 c00, float3 c01, float r0, float3 c10, float3 c11, float r1)
        {
            float3 v0 = c01 - c00;
            float3 v1 = c11 - c10;

            float3 dV = v1 - v0;
            float3 dC = c11 - c01;

            float r2 = (r1 + r0) * (r1 + r0);
            float cc = Dot(dC, dC);

            // -- If the spheres intersect at their end positions then we're done here.
            if(cc < r2) {
                return true;
            }

            float vv = Dot(dV, dV);
            float cv2 = 2.0f * Dot(dC, dV);

            float a = vv;
            float b = cv2;
            float c = cc - r2;

            // -- If a is zero then both spheres are moving at the same velocity. Since we already know they don't currently
            // -- intersect, we know they will forever drift through cold space alone and un-touching.
            if(a < 0.00001f) {
                return false;
            }

            float determinant = b * b - 4.0f * a * c;
            // -- Two spheres walking down the street politely move around each other and say cheers.
            if(determinant < 0.0f) {
                return false;
            }

            float t0 = (-b + determinant) / (2 * a);
            if(t0 >= 0.0f && t0 <= 1.0f) {
                return true;
            }

            float t1 = (-b - determinant) / (2 * a);
            if(t1 >= 0.0f && t1 <= 1.0f) {
                return true;
            }

            // They hit... but not yet. Not now. They wait, silently, for the opportune moment to strike.
            return false;
        }
    }
}