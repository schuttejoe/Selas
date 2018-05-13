#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <UtilityLib/Color.h>
#include <MathLib/FloatStructs.h>
#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    struct SceneContext;

    namespace VCM
    {
        void GenerateImage(SceneContext& context, uint width, uint height, float3* imageData);
    }
}