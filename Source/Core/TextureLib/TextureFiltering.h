#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>
#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    struct TextureResourceData;

    namespace TextureFiltering
    {
        enum WrapMode
        {
            Clamp,
            Repeat
        };

        void InitializeEWAFilterWeights();

        float PointFloat(TextureResourceData* texture, float2 st);
        float TriangleFloat(TextureResourceData* texture, int32 level, float2 st);
        float EWAFloat(TextureResourceData* texture, float2 st, float2 dst0, float2 dst1);

        float3 PointFloat3(TextureResourceData* texture, float2 st);
        float3 TriangleFloat3(TextureResourceData* texture, int32 level, float2 st);
        float3 EWAFloat3(TextureResourceData* texture, float2 st, float2 dst0, float2 dst1);
    }
}