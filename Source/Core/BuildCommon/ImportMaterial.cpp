//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/ImportMaterial.h"
#include "BuildCommon/SceneBuildPipeline.h"
#include "UtilityLib/JsonUtilities.h"
#include "StringLib/StringUtil.h"
#include "SystemLib/MemoryAllocation.h"
#include "SystemLib/CountOf.h"

#include <stdio.h>

namespace Selas
{
    cpointer attributes[] =
    {
        "subsurface",
        "metallic",
        "specular",
        "specularTint",
        "roughness",
        "anisotropic",
        "sheen",
        "sheenTint",
        "clearcoat",
        "clearcoatGloss",
        "specTrans",
        "ior",
        "displacement"
    };
    static_assert(CountOf_(attributes) == eMaterialPropertyCount, "Incorrect attribute string count");

    float attributeDefaults[] =
    {
        0.0f, // subsurface
        0.0f, // metallic
        0.3f, // specular
        0.0f, // specularTint
        0.5f, // roughness
        0.0f, // anisotropic
        0.0f, // sheen
        0.0f, // sheenTint
        0.0f, // clearcoat
        0.0f, // clearcoatGloss
        0.0f, // specTrans
        1.5f, // ior
        0.1f, // displacement
    };
    static_assert(CountOf_(attributeDefaults) == eMaterialPropertyCount, "Incorrect attribute default count");

    Error ImportMaterial(cpointer filepath, ImportedMaterialData* material)
    {
        rapidjson::Document document;
        ReturnError_(Json::OpenJsonDocument(filepath, document));

        Json::ReadFixedString(document, "shaderName", material->shaderName);
        Json::ReadFixedString(document, "baseColorTexture", material->baseColorTexture);
        Json::ReadFixedString(document, "normalTexture", material->normalTexture);

        for(uint scan = 0; scan < eMaterialPropertyCount; ++scan) {
            FixedString64 texPropertyName;
            FixedStringSprintf(texPropertyName, "%sTexture", attributes[scan]);
            Json::ReadFixedString(document, texPropertyName.Ascii(), material->scalarAttributeTextures[scan]);
            Json::ReadFloat(document, attributes[scan], material->scalarAttributes[scan], attributeDefaults[scan]);
        }
             
        Json::ReadBool(document, "alphaTesting", material->alphaTested, false);
        Json::ReadBool(document, "invertDisplacement", material->invertDisplacement, false);

        return Success_;
    }
}