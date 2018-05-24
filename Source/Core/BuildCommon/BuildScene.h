#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Selas
{
    struct ImportedModel;

    struct BuiltScene;
    bool BuildScene(ImportedModel* imported, BuiltScene* built);
}