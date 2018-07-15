//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/ImageBasedLightBuildProcessor.h"

#include "BuildCommon/BuildImageBasedLight.h"
#include "BuildCommon/BakeImageBasedLight.h"

#include "SceneLib/ImageBasedLightResource.h"
#include "Assets/AssetFileUtils.h"
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
        SafeFree_(iblData.hdrData);

        return Success_;
    }
}