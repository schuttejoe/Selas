//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/ImportMaterial.h"
#include "BuildCommon/ModelBuildPipeline.h"
#include "Assets/AssetFileUtils.h"
#include "UtilityLib/JsonUtilities.h"
#include "StringLib/StringUtil.h"
#include "SystemLib/MemoryAllocation.h"
#include "SystemLib/CountOf.h"

#include <stdio.h>

namespace Selas
{
    cpointer attributes[] =
    {
        "metallic",
        "specularTint",
        "roughness",
        "anisotropic",
        "sheen",
        "sheenTint",
        "clearcoat",
        "clearcoatGloss",
        "specTrans",
        "diffTrans",
        "flatness",
        "ior",
        "scatterDistance",
        "displacement"
    };
    static_assert(CountOf_(attributes) == eMaterialPropertyCount, "Incorrect attribute string count");

    float attributeDefaults[] =
    {
        0.3f, // metallic
        0.0f, // specularTint
        0.5f, // roughness
        0.0f, // anisotropic
        0.0f, // sheen
        0.0f, // sheenTint
        0.0f, // clearcoat
        0.0f, // clearcoatGloss
        0.0f, // specTrans
        0.0f, // diffTrans
        0.0f, // flatness
        1.5f, // ior
        0.0f, // scatter distance
        0.0f, // displacement
    };
    static_assert(CountOf_(attributeDefaults) == eMaterialPropertyCount, "Incorrect attribute default count");

    //=============================================================================================================================
    Error ImportMaterial(cpointer filepath, ImportedMaterialData* material)
    {
        rapidjson::Document document;
        ReturnError_(Json::OpenJsonDocument(filepath, document));

        Json::ReadFixedString(document, "shaderName", material->shaderName);
        Json::ReadFixedString(document, "normalTexture", material->normalTexture);

        material->baseColorTexture.Clear();
        if(Json::IsStringAttribute(document, "baseColor")) {
            Json::ReadFixedString(document, "baseColor", material->baseColorTexture);
        }
        else {
            Json::ReadFloat3(document, "baseColor", material->baseColor, float3(0.6f, 0.6f, 0.6f));
        }

        Json::ReadFloat3(document, "transmittanceColor", material->transmittanceColor, float3::Zero_);

        for(uint scan = 0; scan < eMaterialPropertyCount; ++scan) {
            material->scalarAttributeTextures[scan].Clear();

            if(Json::IsStringAttribute(document, attributes[scan])) {
                Json::ReadFixedString(document, attributes[scan], material->scalarAttributeTextures[scan]);
            }
            else {
                Json::ReadFloat(document, attributes[scan], material->scalarAttributes[scan], attributeDefaults[scan]);
            }
        }
             
        Json::ReadBool(document, "alphaTesting", material->alphaTested, false);
        Json::ReadBool(document, "invertDisplacement", material->invertDisplacement, false);

        return Success_;
    }

    //=============================================================================================================================
    Error ImportDisneyMaterials(cpointer filepath, cpointer texturePrefix,
                                CArray<Hash32>& materialHashes, CArray<ImportedMaterialData>& materials)
    {
        rapidjson::Document document;
        ReturnError_(Json::OpenJsonDocument(filepath, document));
        for(const auto& keyvalue : document.GetObject()) {
            const auto& element = keyvalue.value;
            
            cpointer name = keyvalue.name.GetString();
            Hash32 hash = MurmurHash3_x86_32(name, StringUtil::Length(name), 0);

            ImportedMaterialData material;
            material.alphaTested = false;
            material.invertDisplacement = false;
            material.normalTexture.Clear();
            material.baseColorTexture.Clear();

            Json::ReadFloat3(element, "baseColor", material.baseColor, float3(1.0f, 1.0f, 1.0f));
            for(uint scan = 0; scan < eMaterialPropertyCount; ++scan) {
                material.scalarAttributeTextures[scan].Clear();
                Json::ReadFloat(element, attributes[scan], material.scalarAttributes[scan], attributeDefaults[scan]);
            }

            FixedString32 type;
            Json::ReadFixedString(element, "type", type);
            if(StringUtil::Equals(type.Ascii(), "solid"))
                material.shaderName.Copy("DisneySolid");
            else
                material.shaderName.Copy("DisneyThin");

            if(Json::ReadFixedString(element, "colorMap", texturePrefix, material.ptexFolder) == false) {
                return Error_("Failed to find colormap for material %s", name);
            }

            AssetFileUtils::IndependentPathSeperators(material.ptexFolder);

            materials.Add(material);
            materialHashes.Add(hash);
        }

        return Success_;
    }
}