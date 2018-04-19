#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

namespace Shooty
{
    struct float4x4;
    struct float4;

    namespace Math
    {
        void CalculateFrustumPlanes(const float4x4& projection, float4* planes);
    }
}