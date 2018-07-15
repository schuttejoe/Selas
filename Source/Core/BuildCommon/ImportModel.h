#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/SceneBuildPipeline.h"
#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    struct BuildProcessorContext;

    Error ImportModel(BuildProcessorContext* context, ImportedModel* scene);
    void ShutdownImportedModel(ImportedModel* scene);
}