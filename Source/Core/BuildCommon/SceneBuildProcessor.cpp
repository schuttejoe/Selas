//==============================================================================
// Joe Schutte
//==============================================================================

#include "BuildCommon/SceneBuildProcessor.h"

#include "BuildCommon/ImportModel.h"
#include "BuildCommon/BuildScene.h"
#include "BuildCommon/BakeScene.h"
#include "BuildCore/BuildContext.h"
#include "SceneLib/SceneResource.h"
#include "Assets/AssetFileUtils.h"
#include "SystemLib/MemoryAllocation.h"

namespace Selas
{
    //==============================================================================
    Error CSceneBuildProcessor::Setup()
    {
        AssetFileUtils::EnsureAssetDirectory<SceneResource>();

        return Success_;
    }

    //==============================================================================
    cpointer CSceneBuildProcessor::Type()
    {
        // JSTODO - Allow for multiple input types to a build processor.
        return "fbx";
    }

    //==============================================================================
    uint64 CSceneBuildProcessor::Version()
    {
        return SceneResource::kDataVersion;
    }

    //==============================================================================
    Error CSceneBuildProcessor::Process(BuildProcessorContext* context)
    {
        ImportedModel importedModel;
        ReturnError_(ImportModel(context, &importedModel));

        BuiltScene builtScene;
        ReturnError_(BuildScene(context, &importedModel, &builtScene));
        ShutdownImportedModel(&importedModel);

        for(uint scan = 0, count = builtScene.textures.Length(); scan < count; ++scan) {
            cpointer textureName = builtScene.textures[scan].Ascii();
            context->AddProcessDependency("Texture", textureName);
        }

        BakeScene(context, builtScene);

        return Success_;
    }
}