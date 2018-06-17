#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    struct BuildProcessorContext;
    struct ImportedModel;
    struct BuiltScene;

    Error BuildScene(BuildProcessorContext* context, cpointer materialPrefix, ImportedModel* imported, BuiltScene* built);
}