#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "UtilityLib/MurmurHash.h"
#include "ContainersLib/CArray.h"
#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    struct ImportedMaterialData;

    Error ImportMaterial(cpointer filepath, ImportedMaterialData* material);
    Error ImportDisneyMaterials(cpointer filepath, cpointer texturePrefix,
                                CArray<Hash32>& materialHashes, CArray<ImportedMaterialData>& materials);
}