//==============================================================================
// Joe Schutte
//==============================================================================

#include "BuildCommon/ImportMaterial.h"
#include "BuildCommon/SceneBuildPipeline.h"
#include "UtilityLib/JsonUtilities.h"
#include "StringLib/StringUtil.h"
#include "SystemLib/MemoryAllocation.h"

#include <stdio.h>

namespace Selas
{
    Error ImportMaterial(cpointer filepath, ImportedMaterialData* material)
    {
        rapidjson::Document document;
        ReturnError_(Json::OpenJsonDocument(filepath, document));

        Json::ReadFixedString(document, "ShaderName", material->shaderName);

        Json::ReadFixedString(document, "AlbedoTexture", material->albedoTextureName);
        Json::ReadFixedString(document, "HeightTexture", material->heightTextureName);
        Json::ReadFixedString(document, "NormalTexture", material->normalTextureName);
        Json::ReadFixedString(document, "RoughnessTexture", material->roughnessTextureName);
        Json::ReadFixedString(document, "SpecularTexture", material->specularTextureName);
        Json::ReadFixedString(document, "MetalnessTexture", material->metalnessTextureName);

        Json::ReadFloat(document, "Metalness", material->metalness, 1.0f);
        Json::ReadFloat(document, "Albedo", material->albedo, 1.0f);
        Json::ReadFloat(document, "Roughness", material->roughness, 1.0f);
        Json::ReadFloat(document, "Ior", material->ior, 1.0f);

        Json::ReadBool(document, "AlphaTesting", material->alphaTested, false);

        return Success_;
    }
}