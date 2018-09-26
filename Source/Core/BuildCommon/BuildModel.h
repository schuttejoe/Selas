#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    struct MaterialResourceData;
    struct ImportedMaterialData;
    struct BuildProcessorContext;
    struct ImportedModel;
    struct BuiltModel;

    void BuildMaterial(const ImportedMaterialData& importedMaterialData, MaterialResourceData& material);
    Error BuildModel(BuildProcessorContext* context, cpointer materialPrefix, ImportedModel* imported, BuiltModel* built);
}