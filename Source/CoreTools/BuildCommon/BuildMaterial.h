#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Shooty {

    struct MaterialData;

    bool ImportMaterial(const char* materialName, MaterialData* material);
};