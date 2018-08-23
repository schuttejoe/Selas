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
    struct RayCastCameraSettings;

    namespace PathTracer
    {
        void GenerateImage(SceneContext& context, cpointer imageName, const RayCastCameraSettings& camera);
    }
}