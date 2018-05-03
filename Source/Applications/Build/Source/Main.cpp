//=================================================================================================
// Main.cpp
//=================================================================================================

// -- Build
#include <BuildCommon/BuildImageBasedLight.h>
#include <BuildCommon/BakeImageBasedLight.h>
#include <BuildCommon/ImportModel.h>
#include <BuildCommon/BuildScene.h>
#include <BuildCommon/BakeScene.h>
#include <BuildCommon/BuildTexture.h>
#include <BuildCommon/BakeTexture.h>

// -- engine
#include <SceneLib/ImageBasedLightResource.h>
#include <TextureLib/TextureResource.h>
#include <IoLib/File.h>
#include <IoLib/Directory.h>
#include <MathLib/FloatFuncs.h>
#include <MathLib/SphericalHarmonic.h>
#include <SystemLib/MemoryAllocation.h>
#include <SystemLib/JsAssert.h>

// -- clr
#include <stdio.h>
#include <string.h>

using namespace Shooty;

//=================================================================================================
int main(int argc, char *argv[])
{
    Directory::EnsureDirectoryExists("D:\\Shooty\\ShootyEngine\\_Assets\\Scenes\\");
    Directory::EnsureDirectoryExists("D:\\Shooty\\ShootyEngine\\_Assets\\IBLs\\");
    Directory::EnsureDirectoryExists("D:\\Shooty\\ShootyEngine\\_Assets\\Textures\\");

    #define ExportModel_ 1
    #define ExportIbl_ 0

#if ExportModel_
    ImportedModel importedModel;
    if (!ImportModel("D:\\Shooty\\ShootyEngine\\Content\\Meshes\\plane_with_sphere.fbx", &importedModel)) {
        Error_("Error importing model.");
        return -1;
    }

    BuiltScene builtScene;
    if (!BuildScene(&importedModel, &builtScene)) {
        Error_("Error building imported scene");
        return -1;
    }
    ShutdownImportedModel(&importedModel);

    for(uint scan = 0, count = builtScene.textures.Length(); scan < count; ++scan) {
        TextureResourceData textureData;
        cpointer textureName = builtScene.textures[scan].Ascii();

        if(!ImportTexture(textureName, Box, &textureData)) {
            Error_("Error importing texture");
            return -1;
        }

        if(!BakeTexture(&textureData, textureName)) {
            Error_("Error writing texture asset");
            return -1;
        }

        Free_(textureData.texture);
    }

    BakeScene(builtScene, "D:\\Shooty\\ShootyEngine\\_Assets\\Scenes\\plane_with_sphere");
#endif

#if ExportIbl_
    ImageBasedLightResourceData iblData;
    if(!ImportImageBasedLight("D:\\Shooty\\ShootyEngine\\Content\\HDR\\simons_town_rocks_4k.hdr", &iblData)) {
        Error_("Error importing hdr");
        return -1;
    }

    BakeImageBasedLight(&iblData, "D:\\Shooty\\ShootyEngine\\_Assets\\IBLs\\simons_town_rocks_4k.bin");
    SafeFree_(iblData.densityfunctions.conditionalDensityFunctions);
    SafeFree_(iblData.densityfunctions.marginalDensityFunction);
    SafeFree_(iblData.hdrData);
    #endif

    return 0;
}
