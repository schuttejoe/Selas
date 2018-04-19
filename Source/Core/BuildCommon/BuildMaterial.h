#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Shooty {

    struct ImportedMaterialData;

    bool ImportMaterial(const char* materialName, ImportedMaterialData* material);
};