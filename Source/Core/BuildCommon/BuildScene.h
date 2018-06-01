#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/Error.h>
#include <SystemLib/BasicTypes.h>

namespace Selas
{
    struct ImportedModel;

    struct BuiltScene;
    Error BuildScene(ImportedModel* imported, BuiltScene* built);
}