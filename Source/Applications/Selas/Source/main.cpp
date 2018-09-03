
//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "PathTracer.h"
#include "VCM.h"

#include "BuildCommon/ImageBasedLightBuildProcessor.h"
#include "BuildCommon/TextureBuildProcessor.h"
#include "BuildCommon/CModelBuildProcessor.h"
#include "BuildCommon/CDisneySceneBuildProcessor.h"
#include "BuildCommon/CSceneBuildProcessor.h"
#include "BuildCore/BuildCore.h"
#include "BuildCore/BuildDependencyGraph.h"
#include "Shading/IntegratorContexts.h"
#include "SceneLib/SceneResource.h"
#include "SceneLib/ModelResource.h"
#include "SceneLib/ImageBasedLightResource.h"
#include "TextureLib/Framebuffer.h"
#include "TextureLib/TextureFiltering.h"
#include "ThreadingLib/JobMgr.h"
#include "IoLib/Environment.h"
#include "StringLib/FixedString.h"
#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"
#include "SystemLib/SystemTime.h"
#include "SystemLib/Logging.h"

#include "embree3/rtcore.h"
#include "embree3/rtcore_ray.h"

#include "xmmintrin.h"
#include "pmmintrin.h"
#include <stdio.h>

using namespace Selas;

//static cpointer sceneName = "Scenes~TestScene.json";
//static cpointer sceneType = "scene";

static cpointer sceneName = "Scenes~island~island.json";
static cpointer sceneType = "disney";

//=================================================================================================================================
static Error ValidateAssetsAreBuilt()
{
    auto timer = SystemTime::Now();

    CJobMgr* jobMgr = New_(CJobMgr);
    jobMgr->Initialize();

    CBuildDependencyGraph depGraph;
    ReturnError_(depGraph.Initialize());

    CBuildCore buildCore;
    buildCore.Initialize(jobMgr, &depGraph);

    CreateAndRegisterBuildProcessor<CImageBasedLightBuildProcessor>(&buildCore);
    CreateAndRegisterBuildProcessor<CTextureBuildProcessor>(&buildCore);
    CreateAndRegisterBuildProcessor<CModelBuildProcessor>(&buildCore);
    CreateAndRegisterBuildProcessor<CDisneySceneBuildProcessor>(&buildCore);
    CreateAndRegisterBuildProcessor<CSceneBuildProcessor>(&buildCore);

    buildCore.BuildAsset(ContentId(sceneType, sceneName));

    ReturnError_(buildCore.Execute());
    buildCore.Shutdown();

    ReturnError_(depGraph.Shutdown());
    jobMgr->Shutdown();
    Delete_(jobMgr);

    float elapsedMs = SystemTime::ElapsedMillisecondsF(timer);
    WriteDebugInfo_("Scene build time %fms", elapsedMs);

    return Success_;
}

//=================================================================================================================================
int main(int argc, char *argv[])
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    Environment_Initialize(ProjectRootName_, argv[0]);

    TextureFiltering::InitializeEWAFilterWeights();

    ExitMainOnError_(ValidateAssetsAreBuilt());

    RTCDevice rtcDevice = rtcNewDevice(nullptr/*"verbose=3"*/);

    SceneResource sceneResource;

    auto timer = SystemTime::Now();
    ExitMainOnError_(ReadSceneResource(sceneName, &sceneResource));
    ExitMainOnError_(InitializeSceneResource(&sceneResource));
    float elapsedMs = SystemTime::ElapsedMillisecondsF(timer);
    WriteDebugInfo_("Scene load time %fms", elapsedMs);

    timer = SystemTime::Now();
    InitializeEmbreeScene(&sceneResource, rtcDevice);
    elapsedMs = SystemTime::ElapsedMillisecondsF(timer);
    WriteDebugInfo_("Embree setup time %fms", elapsedMs);

    Selas::uint width = 1920;
    Selas::uint height = 800;

    RayCastCameraSettings camera;
    InitializeSceneCamera(&sceneResource, width, height, camera);

    timer = SystemTime::Now();
    PathTracer::GenerateImage(&sceneResource, camera, "UnidirectionalPT");
    //VCM::GenerateImage(&sceneResource, camera, "VCM");
    elapsedMs = SystemTime::ElapsedMillisecondsF(timer);
    WriteDebugInfo_("Scene render time %fms", elapsedMs);

    ShutdownSceneResource(&sceneResource);
    rtcReleaseDevice(rtcDevice);

    return 0;
}
