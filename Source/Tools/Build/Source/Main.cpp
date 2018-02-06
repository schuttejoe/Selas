//=================================================================================================
// Main.cpp
//=================================================================================================

// -- Build
#include <BuildCommon/ImportScene.h>
#include <BuildCommon/BuildScene.h>
#include <BuildCommon/BakeScene.h>

// -- engine
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
    ImportedScene importedScene;
    if (!ImportScene("D:\\Shooty\\ShootyEngine\\content\\Meshes\\teapot.obj", &importedScene)) {
        Error_("Error importing teapot.obj");
        return -1;
    }

    BuiltScene builtScene;
    if (!BuildScene(&importedScene, &builtScene)) {
        Error_("Error building imported scene");
        return -1;
    }
    ShutdownImportedScene(&importedScene);

    BakeScene(builtScene, "D:\\Shooty\\ShootyEngine\\_Assets\\scene.bin");

    return 0;
}
