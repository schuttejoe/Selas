#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "UtilityLib/Color.h"
#include "MathLib/FloatStructs.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    struct RayCastCameraSettings;
    struct SceneResource;

    namespace VCM
    {
        void GenerateImage(SceneResource* scene, const RayCastCameraSettings& camera, cpointer imageName);
    }
}