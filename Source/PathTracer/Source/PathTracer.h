#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <UtilityLib/Color.h>
#include <MathLib/FloatStructs.h>
#include <SystemLib/BasicTypes.h>

struct RTCSceneTy;
typedef struct RTCSceneTy* RTCScene;

namespace Shooty
{
    struct SceneResource;
    struct ImageBasedLightResourceData;
    struct TextureResourceData;

    struct SceneContext
    {
        RTCScene rtcScene;
        SceneResource* scene;
        TextureResourceData* textures;
        ImageBasedLightResourceData* ibl;
        uint width;
        uint height;
    };  

    void PathTraceImage(const SceneContext& context, float3* imageData);
}