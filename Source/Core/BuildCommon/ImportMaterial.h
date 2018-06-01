#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/Error.h>
#include <SystemLib/BasicTypes.h>

namespace Selas
{
    struct ImportedMaterialData;

    Error ImportMaterial(cpointer materialName, ImportedMaterialData* material);
}