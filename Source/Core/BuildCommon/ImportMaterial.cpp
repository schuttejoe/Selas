//==============================================================================
// Joe Schutte
//==============================================================================

#include <BuildCommon/ImportMaterial.h>
#include <BuildCommon/SceneBuildPipeline.h>
#include <UtilityLib/JsonUtilities.h>
#include <StringLib/StringUtil.h>
#include <SystemLib/MemoryAllocation.h>

#include <stdio.h>

namespace Shooty
{
    // JSTODO - Set up the same environment class I did at Sparkypants. Except make it relative and not depend on the environment variable.
    cpointer MaterialBaseDirectory = "D:\\Shooty\\ShootyEngine\\Content\\Materials\\";

    bool ImportMaterial(cpointer materialName, ImportedMaterialData* material)
    {
        FixedString512 filepath;
        sprintf_s(filepath.Ascii(), filepath.Capcaity(), "%s%s.json", MaterialBaseDirectory, materialName);

        rapidjson::Document document;
        ReturnFailure_(Json::OpenJsonDocument(filepath.Ascii(), document));

        Json::ReadFixedString(document, "ShaderName", material->shaderName);

        Json::ReadFixedString(document, "EmissiveTexture", material->emissiveTextureName);
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

        return true;
    }
}