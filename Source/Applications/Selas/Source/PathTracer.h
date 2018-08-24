#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "UtilityLib/Color.h"
#include "MathLib/FloatStructs.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    struct SceneResource;
    struct RayCastCameraSettings;

    namespace PathTracer
    {
        void GenerateImage(SceneResource* scene, const RayCastCameraSettings& camera, cpointer imageName);
    }
}