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
    void PathTraceImage(SceneContext& context, uint width, uint height, float3* imageData);
}