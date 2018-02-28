//=================================================================================================
// Main.cpp
//=================================================================================================

// -- Build
#include <BuildCommon/BuildImageBasedLight.h>
#include <BuildCommon/BakeImageBasedLight.h>
#include <BuildCommon/ImportScene.h>
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
    Directory::CreateDirectoryTree("D:\\Shooty\\ShootyEngine\\_Assets\\Scenes\\");
    Directory::CreateDirectoryTree("D:\\Shooty\\ShootyEngine\\_Assets\\IBLs\\");
    Directory::CreateDirectoryTree("D:\\Shooty\\ShootyEngine\\_Assets\\Textures\\");

    #define ExportScene_ 0
    #define ExportIbl_ 1
    #define ExportTextures_ 0

#if ExportScene_
    ImportedScene importedScene;
    if (!ImportScene("D:\\Shooty\\ShootyEngine\\Content\\Meshes\\sphere.fbx", &importedScene)) {
        Error_("Error importing obj");
        return -1;
    }

    BuiltScene builtScene;
    if (!BuildScene(&importedScene, &builtScene)) {
        Error_("Error building imported scene");
        return -1;
    }
    ShutdownImportedScene(&importedScene);

    BakeScene(builtScene, "D:\\Shooty\\ShootyEngine\\_Assets\\Scenes\\sphere");
#endif

#if ExportIbl_
    ImageBasedLightResourceData iblData;
    if(!ImportImageBasedLight("D:\\Shooty\\ShootyEngine\\Content\\HDR\\red_wall_1k.hdr", &iblData)) {
        Error_("Error importing hdr");
        return -1;
    }

    BakeImageBasedLight(&iblData, "D:\\Shooty\\ShootyEngine\\_Assets\\IBLs\\red_wall_1k");
    SafeFree_(iblData.densityfunctions.conditionalDensityFunctions);
    SafeFree_(iblData.densityfunctions.marginalDensityFunction);
    SafeFree_(iblData.hdrData);
#endif

#if ExportTextures_
    TextureResourceData textureData;
    if(!ImportTexture("D:\\Shooty\\ShootyEngine\\Content\\HDR\\red_wall_4k.hdr", Box, &textureData)) {
        Error_("Error importing texture");
        return -1;
    }
    if(!BakeTexture(&textureData, "D:\\Shooty\\ShootyEngine\\_Assets\\Textures\\red_wall_4k"))
    Free_(textureData.mipmaps);

#endif

    return 0;
}
