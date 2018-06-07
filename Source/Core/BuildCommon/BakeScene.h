#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "SystemLib/Error.h"
#include "BuildCommon/SceneBuildPipeline.h"

namespace Selas
{
    //==============================================================================
    Error BakeScene(const BuiltScene& scene_data, cpointer filepath);
}