#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Selas
{
    struct ImportedMaterialData;

    bool ImportMaterial(cpointer materialName, ImportedMaterialData* material);
}