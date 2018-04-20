#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    struct ImportedModel;

    struct BuiltScene;
    bool BuildScene(ImportedModel* imported, BuiltScene* built);
}