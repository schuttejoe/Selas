//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/TextureBuildProcessor.h"
#include "BuildCommon/BuildTexture.h"
#include "BuildCommon/BakeTexture.h"
#include "TextureLib/TextureResource.h"
#include "Assets/AssetFileUtils.h"
#include "SystemLib/MemoryAllocation.h"

namespace Selas
{
    //=============================================================================================================================
    Error CTextureBuildProcessor::Setup()
    {
        AssetFileUtils::EnsureAssetDirectory<TextureResource>();

        return Success_;
    }

    //=============================================================================================================================
    cpointer CTextureBuildProcessor::Type()
    {
        return "Texture";
    }

    //=============================================================================================================================
    uint64 CTextureBuildProcessor::Version()
    {
        return TextureResource::kDataVersion;
    }

    //=============================================================================================================================
    Error CTextureBuildProcessor::Process(BuildProcessorContext* context)
    {
        TextureResourceData textureData;
        ReturnError_(ImportTexture(context, Box, &textureData));
        ReturnError_(BakeTexture(context, &textureData));

        Free_(textureData.texture);

        return Success_;
    }
}