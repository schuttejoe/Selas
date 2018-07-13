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
        Json::ReadFixedString(document, "DisplacementTexture", material->displacementTextureName);
        Json::ReadFixedString(document, "NormalTexture", material->normalTextureName);
        Json::ReadFixedString(document, "RoughnessTexture", material->roughnessTextureName);
        Json::ReadFixedString(document, "SpecularTexture", material->specularTextureName);
        Json::ReadFixedString(document, "MetalnessTexture", material->metalnessTextureName);

        Json::ReadFloat(document, "Metalness", material->metalness, 0.1f);
        Json::ReadFloat(document, "Albedo", material->albedo, 0.4f);
        Json::ReadFloat(document, "Roughness", material->roughness, 0.4f);
        Json::ReadFloat(document, "Ior", material->ior, 1.0f);
        Json::ReadFloat(document, "DisplacementScale", material->displacementScale, 0.1f);        

        Json::ReadBool(document, "AlphaTesting", material->alphaTested, false);
        Json::ReadBool(document, "InvertDisplacement", material->invertDisplacement, false);
        
        Json::ReadFloat(document, "Subsurface", material->subsurface, 0.0f);

        return Success_;
    }
}