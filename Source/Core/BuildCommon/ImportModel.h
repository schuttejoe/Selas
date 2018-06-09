#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "BuildCommon/SceneBuildPipeline.h"
#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    Error ImportModel(const char* filename, ImportedModel* scene);
    void ShutdownImportedModel(ImportedModel* scene);
}