#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "FloatStructs.h"
#include <SystemLib/BasicTypes.h>

namespace Shooty {
    namespace Intersection {

        bool RaySphere(float3 origin, float3 direction, float3 sphereCenter, float sphereRadius);
        bool RayAABox(float3 ray_origin, float3 ray_direction, float3 min_point, float3 max_point);
        bool SweptSphereSphere(float3 c00, float3 c01, float r0, float3 c10, float3 c11, float r1);
    }
}
