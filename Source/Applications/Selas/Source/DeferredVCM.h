#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "UtilityLib/Color.h"
#include "MathLib/FloatStructs.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    struct SceneContext;
    struct Framebuffer;

    namespace DeferredVCM
    {
        void GenerateImage(SceneContext& context, Framebuffer* frame);
    }
}