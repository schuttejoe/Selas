#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <UtilityLib/Color.h>
#include <MathLib/FloatStructs.h>
#include <SystemLib/BasicTypes.h>

namespace Selas
{
    struct SceneContext;

    namespace PathTracer
    {
        void GenerateImage(SceneContext& context, uint width, uint height, float3* imageData);
    }
}