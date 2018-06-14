//=================================================================================================
// Main.cpp
//=================================================================================================

// -- Build
#include "BuildCommon/ImageBasedLightBuildProcessor.h"
#include "BuildCommon/TextureBuildProcessor.h"
#include "BuildCommon/SceneBuildProcessor.h"
#include "BuildCore/BuildCore.h"
#include "BuildCore/BuildDependencyGraph.h"

// -- engine
#include "ThreadingLib/JobMgr.h"
#include "IoLib/Environment.h"

// -- clr
#include <stdio.h>

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

    buildCore.BuildAsset(ContentId("fbx", "Meshes~plane_with_sphere.fbx"));
    buildCore.BuildAsset(ContentId("HDR", "HDR~simons_town_rocks_4k_upper.hdr"));

    ExitMainOnError_(buildCore.Execute());
    buildCore.Shutdown();

    ExitMainOnError_(depGraph.Shutdown());
    jobMgr.Shutdown();

    return 0;
}
