#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <BuildCommon\SceneBuildPipeline.h>
#include <SystemLib\BasicTypes.h>

namespace Shooty {

    bool BuildScene(ImportedScene* imported, BuiltScene* built);
    void ShutdownBuiltScene(BuiltScene* built);
}