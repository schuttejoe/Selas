#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================
#include "MathLib/FloatStructs.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    namespace Math
    {
        // -- Cubemap
        float3 CubemapToCartesian(uint face, float u, float v);
        float3 CartesianToCubemap(const float3& xyz); // returns (face, u, v)

        // -- Spherical
        void NormalizedCartesianToSpherical(const float3& v, float& theta, float& phi);
        float3 SphericalToCartesian(const float3& rthetaphi);
        float3 SphericalToCartesian(float theta, float phi);
    }
}