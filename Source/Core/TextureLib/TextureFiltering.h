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

        float3 Point(TextureResourceData* texture, float2 st);

        float3 Triangle(TextureResourceData* texture, int32 level, float2 st);
        float3 EWA(TextureResourceData* texture, float2 st, float2 dst0, float2 dst1);
    }
}