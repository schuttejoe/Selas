#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "UtilityLib/MurmurHash.h"
#include "StringLib/FixedString.h"
#include "ContainersLib/CArray.h"
#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    struct ImportedMaterialData;

    Error ImportMaterial(cpointer filepath, ImportedMaterialData* material);
    Error ImportDisneyMaterials(cpointer filepath, CArray<FixedString64>& materialNames, CArray<ImportedMaterialData>& materials);
}