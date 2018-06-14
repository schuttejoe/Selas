#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "SystemLib/Error.h"
#include "BuildCommon/SceneBuildPipeline.h"

namespace Selas
{
    struct BuildProcessorContext;

    //==============================================================================
    Error BakeScene(BuildProcessorContext* context, const BuiltScene& scene_data);
}