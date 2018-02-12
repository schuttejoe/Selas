#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>

struct RTCSceneTy;
typedef struct RTCSceneTy* RTCScene;

namespace Shooty
{
    struct SceneResourceData;
    struct ImageBasedLightResourceData;

    struct SceneContext
    {
        RTCScene rtcScene;
        SceneResourceData* scene;
        ImageBasedLightResourceData* ibl;
        uint width;
        uint height;
    };

    void GenerateRayCastImage(const SceneContext& context, uint32* imageData);
}