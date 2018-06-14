//=================================================================================================
// Main.cpp
//=================================================================================================

// -- Build
#include "BuildCore/BuildCore.h"
#include "BuildCore/BuildDependencyGraph.h"

#include "BuildCommon/ImageBasedLightBuildProcessor.h"
#include "BuildCommon/TextureBuildProcessor.h"
#include "BuildCommon/SceneBuildProcessor.h"

// -- engine
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
#include <stdlib.h>

using namespace Selas;

//=================================================================================================
int main(int argc, char *argv[])
{
    Environment_Initialize(ProjectRootName_, argv[0]);

    CJobMgr jobMgr;
    jobMgr.Initialize();

    CBuildDependencyGraph depGraph;
    ExitMainOnError_(depGraph.Initialize());

    CBuildCore buildCore;
    buildCore.Initialize(&jobMgr, &depGraph);

    CreateAndRegisterBuildProcessor<CImageBasedLightBuildProcessor>(&buildCore);
    CreateAndRegisterBuildProcessor<CTextureBuildProcessor>(&buildCore);
    CreateAndRegisterBuildProcessor<CSceneBuildProcessor>(&buildCore);

    buildCore.BuildAsset(ContentId("fbx", "Meshes|plane_with_sphere.fbx"));

    ExitMainOnError_(buildCore.Execute());
    buildCore.Shutdown();

    ExitMainOnError_(depGraph.Shutdown());
    jobMgr.Shutdown();

    return 0;
}
