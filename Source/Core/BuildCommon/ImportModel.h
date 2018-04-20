#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <BuildCommon/SceneBuildPipeline.h>
#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    bool ImportModel(const char* filename, ImportedModel* scene);
    void ShutdownImportedModel(ImportedModel* scene);
}