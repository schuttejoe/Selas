//==============================================================================
// Joe Schutte
//==============================================================================

#include "BuildCommon/ImageBasedLightBuildProcessor.h"

#include "BuildCommon/BuildImageBasedLight.h"
#include "BuildCommon/BakeImageBasedLight.h"

#include "SceneLib/ImageBasedLightResource.h"
#include "Assets/AssetFileUtils.h"
#include "SystemLib/MemoryAllocation.h"

namespace Selas
{
    //==============================================================================
    Error CImageBasedLightBuildProcessor::Setup()
    {
        AssetFileUtils::EnsureAssetDirectory("IBL");

        return Success_;
    }

    //==============================================================================
    cpointer CImageBasedLightBuildProcessor::Type()
    {
        return "IBL";
    }

    //==============================================================================
    uint64 CImageBasedLightBuildProcessor::Version()
    {
        return 1528575978ul;
    }

    //==============================================================================
    Error CImageBasedLightBuildProcessor::Process(BuildProcessorContext* context)
    {
        ImageBasedLightResourceData iblData;
        ReturnError_(ImportImageBasedLight("D:\\Shooty\\Selas\\Content\\HDR\\simons_town_rocks_4k_upper.hdr", &iblData));
        ReturnError_(BakeImageBasedLight(&iblData, "D:\\Shooty\\Selas\\_Assets\\IBLs\\simons_town_rocks_4k_upper.bin"));

        SafeFree_(iblData.densityfunctions.conditionalDensityFunctions);
        SafeFree_(iblData.densityfunctions.marginalDensityFunction);
        SafeFree_(iblData.hdrData);

        return Success_;
    }
}