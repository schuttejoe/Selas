
//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "PathTracer.h"
#include "DeferredPathTracer.h"
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
#include "SceneLib/GeometryCache.h"
#include "TextureLib/TextureCache.h"
#include "TextureLib/Framebuffer.h"
#include "TextureLib/TextureFiltering.h"
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

#define TextureCacheSize_   4 * 1024 * 1024 * 1024ull
#define GeometryCacheSize_ 18 * 1024 * 1024 * 1024ull

using namespace Selas;

//static cpointer sceneName = "Scenes~TestScene.json";
//static cpointer sceneType = "scene";

static cpointer sceneName = "Scenes~island~island.json";
static cpointer sceneType = "disneyscene";

//=================================================================================================================================
static Error ValidateAssetsAreBuilt()
{
    auto timer = SystemTime::Now();

    CBuildDependencyGraph depGraph;
    ReturnError_(depGraph.Initialize());

    CBuildCore buildCore;
    buildCore.Initialize(&depGraph);

    CreateAndRegisterBuildProcessor<CImageBasedLightBuildProcessor>(&buildCore);
    CreateAndRegisterBuildProcessor<CDualImageBasedLightBuildProcessor>(&buildCore);
    CreateAndRegisterBuildProcessor<CTextureBuildProcessor>(&buildCore);
    CreateAndRegisterBuildProcessor<CModelBuildProcessor>(&buildCore);
    CreateAndRegisterBuildProcessor<CDisneySceneBuildProcessor>(&buildCore);
    CreateAndRegisterBuildProcessor<CSceneBuildProcessor>(&buildCore);

    buildCore.BuildAsset(ContentId(sceneType, sceneName));

    ReturnError_(buildCore.Execute());
    buildCore.Shutdown();

    ReturnError_(depGraph.Shutdown());

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

    TextureCache textureCache;
    textureCache.Initialize(TextureCacheSize_);

    GeometryCache geometryCache;
    geometryCache.Initialize(GeometryCacheSize_);

    TextureFiltering::InitializeEWAFilterWeights();

    ExitMainOnError_(ValidateAssetsAreBuilt());

    RTCDevice rtcDevice = rtcNewDevice(nullptr/*"verbose=3"*/);

    SceneResource sceneResource;

    auto timer = SystemTime::Now();
    ExitMainOnError_(ReadSceneResource(sceneName, &sceneResource));
    InitializeSceneResource(&sceneResource, &textureCache, &geometryCache, rtcDevice);
    float elapsedMs = SystemTime::ElapsedMillisecondsF(timer);
    WriteDebugInfo_("Scene load time %fms", elapsedMs);

    geometryCache.RegisterSubscenes(sceneResource.subscenes, sceneResource.data->subsceneNames.Count());

    // -- preload these so they remain always loaded.
    geometryCache.PreloadSubscene("Scenes~island~json~isMountainA~isMountainA.json_geometry");
    geometryCache.PreloadSubscene("Scenes~island~json~isMountainA~isMountainA.json");
    geometryCache.PreloadSubscene("Scenes~island~json~isMountainB~isMountainB.json_geometry");
    geometryCache.PreloadSubscene("Scenes~island~json~isMountainB~isMountainB.json");
    geometryCache.PreloadSubscene("Scenes~island~json~isIronwoodB~isIronwoodB.json_geometry");
    geometryCache.PreloadSubscene("Scenes~island~json~isIronwoodB~isIronwoodB.json");
    geometryCache.PreloadSubscene("Scenes~island~json~isIronwoodA1~isIronwoodA1.json_geometry");
    geometryCache.PreloadSubscene("Scenes~island~json~isIronwoodA1~isIronwoodA1.json");

    Selas::uint width = 1024;
    Selas::uint height = 429;

    for(uint scan = 0, count = sceneResource.data->cameras.Count(); scan < count; ++scan) {
        RayCastCameraSettings camera;
        SetupSceneCamera(&sceneResource, scan, width, height, camera);

        timer = SystemTime::Now();
        //PathTracer::GenerateImage(&geometryCache, &textureCache, &sceneResource, camera, "UnidirectionalPT");
        DeferredPathTracer::GenerateImage(&geometryCache, &textureCache, &sceneResource, camera,
                                          sceneResource.data->cameras[scan].name.Ascii());
        //VCM::GenerateImage(&sceneResource, camera, "VCM");
        elapsedMs = SystemTime::ElapsedMillisecondsF(timer);
        WriteDebugInfo_("Scene render time %fms", elapsedMs);
    }

    ShutdownSceneResource(&sceneResource, &textureCache);
    rtcReleaseDevice(rtcDevice);

    geometryCache.Shutdown();
    textureCache.Shutdown();

    return 0;
}
