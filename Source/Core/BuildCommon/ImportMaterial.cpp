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

        Json::ReadFixedString(document, "shaderName", material->shaderName);

        Json::ReadFixedString(document, "emissive", material->emissive);
        Json::ReadFixedString(document, "albedo", material->albedo);
        Json::ReadFixedString(document, "height", material->height);
        Json::ReadFixedString(document, "normal", material->normal);
        Json::ReadFixedString(document, "roughness", material->roughness);
        Json::ReadFixedString(document, "specular", material->specular);
        Json::ReadFixedString(document, "metalness", material->metalness);

        Json::ReadFloat(document, "MetalnessScale", material->metalnessScale, 1.0f);
        Json::ReadFloat(document, "ior", material->ior, 1.0f);

        return true;
    }
}