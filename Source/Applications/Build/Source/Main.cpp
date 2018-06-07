//=================================================================================================
// Main.cpp
//=================================================================================================

// -- Build
#include "BuildCore/BuildCore.h"

#include "BuildCommon/BuildImageBasedLight.h"
#include "BuildCommon/BakeImageBasedLight.h"
#include "BuildCommon/ImportModel.h"
#include "BuildCommon/BuildScene.h"
#include "BuildCommon/BakeScene.h"
#include "BuildCommon/BuildTexture.h"
#include "BuildCommon/BakeTexture.h"

// -- engine
#include "SceneLib/ImageBasedLightResource.h"
#include "TextureLib/TextureResource.h"
#include "ThreadingLib/JobMgr.h"
#include "IoLib/File.h"
#include "IoLib/Directory.h"
#include "IoLib/Environment.h"
#include "MathLib/FloatFuncs.h"
#include "MathLib/SphericalHarmonic.h"
#include "SystemLib/MemoryAllocation.h"
#include "SystemLib/JsAssert.h"

// -- clr
#include <stdio.h>
#include <string.h>

using namespace Selas;

//=================================================================================================
int main(int argc, char *argv[])
{
    Environment_Initialize(ProjectRootName_, argv[0]);

    CJobMgr jobMgr;
    jobMgr.Initialize();

    CBuildCore buildCore;
    buildCore.Initialize(&jobMgr);

    Directory::EnsureDirectoryExists("D:\\Shooty\\Selas\\_Assets\\Scenes\\");
    Directory::EnsureDirectoryExists("D:\\Shooty\\Selas\\_Assets\\IBLs\\");
    Directory::EnsureDirectoryExists("D:\\Shooty\\Selas\\_Assets\\Textures\\");

    #define ExportModel_ 0
    #define ExportIbl_ 0

#if ExportModel_
    ImportedModel importedModel;
    ExitMainOnError_(ImportModel("D:\\Shooty\\Selas\\Content\\Meshes\\plane_with_sphere.fbx", &importedModel));

    BuiltScene builtScene;    
    ExitMainOnError_(BuildScene(&importedModel, &builtScene));
    ShutdownImportedModel(&importedModel);

    for(uint scan = 0, count = builtScene.textures.Length(); scan < count; ++scan) {
        TextureResourceData textureData;
        cpointer textureName = builtScene.textures[scan].Ascii();

        ExitMainOnError_(ImportTexture(textureName, Box, &textureData));
        ExitMainOnError_(BakeTexture(&textureData, textureName));

        Free_(textureData.texture);
    }

    BakeScene(builtScene, "D:\\Shooty\\Selas\\_Assets\\Scenes\\plane_with_sphere.bin");
#endif

#if ExportIbl_
    ImageBasedLightResourceData iblData;
    ExitMainOnError_(ImportImageBasedLight("D:\\Shooty\\Selas\\Content\\HDR\\simons_town_rocks_4k_upper.hdr", &iblData));
    ExitMainOnError_(BakeImageBasedLight(&iblData, "D:\\Shooty\\Selas\\_Assets\\IBLs\\simons_town_rocks_4k_upper.bin"));

    SafeFree_(iblData.densityfunctions.conditionalDensityFunctions);
    SafeFree_(iblData.densityfunctions.marginalDensityFunction);
    SafeFree_(iblData.hdrData);
#endif

    buildCore.Shutdown();
    jobMgr.Shutdown();

    return 0;
}
