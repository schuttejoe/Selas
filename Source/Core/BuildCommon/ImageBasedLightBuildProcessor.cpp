//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/ImageBasedLightBuildProcessor.h"
#include "BuildCommon/BuildImageBasedLight.h"
#include "BuildCommon/BakeImageBasedLight.h"
#include "BuildCore/BuildContext.h"

#include "UtilityLib/JsonUtilities.h"
#include "SceneLib/ImageBasedLightResource.h"
#include "Assets/AssetFileUtils.h"
#include "MathLib/Trigonometric.h"
#include "SystemLib/MemoryAllocation.h"

namespace Selas
{
    //=============================================================================================================================
    Error CImageBasedLightBuildProcessor::Setup()
    {
        AssetFileUtils::EnsureAssetDirectory<ImageBasedLightResource>();

        return Success_;
    }

    //=============================================================================================================================
    cpointer CImageBasedLightBuildProcessor::Type()
    {
        return "HDR";
    }

    //=============================================================================================================================
    uint64 CImageBasedLightBuildProcessor::Version()
    {
        return ImageBasedLightResource::kDataVersion;
    }

    //=============================================================================================================================
    Error CImageBasedLightBuildProcessor::Process(BuildProcessorContext* context)
    {
        ImageBasedLightResourceData iblData;
        ReturnError_(ImportImageBasedLight(context, &iblData));
        ReturnError_(BakeImageBasedLight(context, &iblData));

        SafeFree_(iblData.densityfunctions.conditionalDensityFunctions);
        SafeFree_(iblData.densityfunctions.marginalDensityFunction);
        SafeFree_(iblData.lightData);

        return Success_;
    }

    //=============================================================================================================================
    static Error ParseDualIblFile(BuildProcessorContext* context,
                                  FilePathString& lightFile, FilePathString& missFile, float& rotationDegrees, float& exposure)
    {
        FilePathString filepath;
        AssetFileUtils::ContentFilePath(context->source.name.Ascii(), filepath);
        context->AddFileDependency(filepath.Ascii());

        rapidjson::Document document;
        ReturnError_(Json::OpenJsonDocument(filepath.Ascii(), document));

        if(Json::ReadFixedString(document, "light", lightFile) == false) {
            return Error_("Ibl file (%s) does not contain a light element", filepath.Ascii());
        }

        if(Json::ReadFixedString(document, "miss", missFile) == false) {
            return Error_("Ibl file (%s) does not contain a miss element", filepath.Ascii());
        }

        Json::ReadFloat(document, "rotationDegrees", rotationDegrees, 0.0f);
        Json::ReadFloat(document, "exposure", exposure, 0.0f);

        return Success_;
    }

    //=============================================================================================================================
    Error CDualImageBasedLightBuildProcessor::Setup()
    {
        AssetFileUtils::EnsureAssetDirectory<ImageBasedLightResource>();

        return Success_;
    }

    //=============================================================================================================================
    cpointer CDualImageBasedLightBuildProcessor::Type()
    {
        return "DualIbl";
    }

    //=============================================================================================================================
    uint64 CDualImageBasedLightBuildProcessor::Version()
    {
        return ImageBasedLightResource::kDataVersion;
    }

    //=============================================================================================================================
    Error CDualImageBasedLightBuildProcessor::Process(BuildProcessorContext* context)
    {
        FilePathString lightFile;
        FilePathString missFile;
        float rotationDegrees;
        float exposure;
        ReturnError_(ParseDualIblFile(context, lightFile, missFile, rotationDegrees, exposure));

        ImageBasedLightResourceData iblData;
        Memory::Zero(&iblData, sizeof(iblData));

        ReturnError_(ImportDualImageBasedLight(context, lightFile.Ascii(), missFile.Ascii(), &iblData));
        iblData.rotationRadians = rotationDegrees * Math::DegreesToRadians_;
        iblData.exposureScale = Math::Powf(2.0f, exposure);
        ReturnError_(BakeImageBasedLight(context, &iblData));

        SafeFree_(iblData.densityfunctions.conditionalDensityFunctions);
        SafeFree_(iblData.densityfunctions.marginalDensityFunction);
        SafeFree_(iblData.lightData);
        SafeFree_(iblData.missData);

        return Success_;
    }
}