//=================================================================================================
// Main.cpp
//=================================================================================================

// -- Build
#include <BuildCommon/BuildImageBasedLight.h>
#include <BuildCommon/BakeImageBasedLight.h>
#include <BuildCommon/ImportScene.h>
#include <BuildCommon/BuildScene.h>
#include <BuildCommon/BakeScene.h>

// -- engine
#include <SceneLib/ImageBasedLightResource.h>
#include <IoLib/File.h>
#include <MathLib/FloatFuncs.h>
#include <MathLib/SphericalHarmonic.h>
#include <SystemLib/MemoryAllocation.h>
#include <SystemLib/JsAssert.h>

// -- clr
#include <stdio.h>
#include <string.h>

using namespace Shooty;

//=================================================================================================
int main(int argc, char *argv[]) {

    #define ExportScene_ 1
    #define ExportIbl_ 0

#if ExportScene_
    ImportedScene importedScene;
    if (!ImportScene("D:\\Shooty\\ShootyEngine\\Content\\Meshes\\living_room_open_windows.fbx", &importedScene)) {
        Error_("Error importing obj");
        return -1;
    }

    BuiltScene builtScene;
    if (!BuildScene(&importedScene, &builtScene)) {
        Error_("Error building imported scene");
        return -1;
    }
    ShutdownImportedScene(&importedScene);

    BakeScene(builtScene, "D:\\Shooty\\ShootyEngine\\_Assets\\scene.bin");
#endif

#if ExportIbl_
    ImageBasedLightResourceData iblData;
    if(!ImportImageBasedLight("D:\\Shooty\\ShootyEngine\\Content\\HDR\\red_wall_1k.hdr", &iblData)) {
        Error_("Error importing hdr");
        return -1;
    }

    BakeImageBasedLight(&iblData, "D:\\Shooty\\ShootyEngine\\_Assets\\ibl.bin");
    SafeFree_(iblData.densityfunctions.conditionalDensityFunctions);
    SafeFree_(iblData.densityfunctions.marginalDensityFunction);
    SafeFree_(iblData.hdrData);
#endif
    return 0;
}
