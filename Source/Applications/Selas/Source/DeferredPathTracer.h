#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "UtilityLib/Color.h"
#include "MathLib/FloatStructs.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    class GeometryCache;
    class TextureCache;
    struct SceneResource;
    struct RayCastCameraSettings;

    namespace DeferredPathTracer
    {
        void GenerateImage(GeometryCache* geometryCache, TextureCache* textureCache, SceneResource* scene,
                           const RayCastCameraSettings& camera, cpointer imageName);
    }
}