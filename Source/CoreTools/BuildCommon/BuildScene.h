#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib\BasicTypes.h>

namespace Shooty {

    struct ImportedScene;
    struct BuiltScene;
    bool BuildScene(ImportedScene* imported, BuiltScene* built);
}