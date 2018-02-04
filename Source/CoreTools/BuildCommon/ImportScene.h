#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <BuildCommon\SceneBuildPipeline.h>
#include <SystemLib\BasicTypes.h>

namespace Shooty {

    bool ImportScene(const char* filename, ImportedScene* scene);
    void ShutdownImportedScene(ImportedScene* scene);
};