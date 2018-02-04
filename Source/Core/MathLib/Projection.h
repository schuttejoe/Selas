#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================
#include "FloatStructs.h"
#include <SystemLib/BasicTypes.h>

namespace Shooty {

    namespace Math {

        // -- Cubemap
        float3 CubemapToCartesian(uint face, float u, float v);
        float3 CartesianToCubemap(const float3& xyz); // returns (face, u, v)

        // -- Spherical
        float3 CartesianToSpherical(const float3& xyz);
        float3 SphericalToCartesian(const float3& rthetaphi);
    }
}