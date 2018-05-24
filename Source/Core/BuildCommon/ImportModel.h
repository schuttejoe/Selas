#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <BuildCommon/SceneBuildPipeline.h>
#include <SystemLib/BasicTypes.h>

namespace Selas
{
    bool ImportModel(const char* filename, ImportedModel* scene);
    void ShutdownImportedModel(ImportedModel* scene);
}