#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "UtilityLib/Color.h"
#include "MathLib/FloatStructs.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    struct SceneContext;
    struct Framebuffer;

    namespace VCM
    {
        void GenerateImage(SceneContext& context, cpointer imageName, uint width, uint height);
    }
}