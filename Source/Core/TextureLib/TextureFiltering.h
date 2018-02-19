#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>
#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    struct TextureResourceData;

    float3 PointSampleTexture(TextureResourceData* texture, float2 uvs);
}