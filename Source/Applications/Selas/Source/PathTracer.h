#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "UtilityLib/Color.h"
#include "MathLib/FloatStructs.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    class TextureCache;
    struct SceneResource;
    struct RayCastCameraSettings;

    namespace PathTracer
    {
        void GenerateImage(TextureCache* cache, SceneResource* scene, const RayCastCameraSettings& camera, cpointer imageName);
    }
}