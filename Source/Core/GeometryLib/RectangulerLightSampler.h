#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>

namespace Selas
{
    struct RectangleLightSampler
    {
        float3 o;
        float3 x, y, z;

        float z0, z0sq;
        float x0, y0, y0sq;
        float x1, y1, y1sq;
        float b0, b1, b0sq, k;
        float S;
    };

    void InitializeRectangleLightSampler(float3 s, float3 eX, float3 eY, float3 o, RectangleLightSampler& sampler);
    float3 SampleRectangleLight(const RectangleLightSampler& sampler, float u, float v);
}