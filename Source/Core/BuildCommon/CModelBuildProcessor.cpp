//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/CModelBuildProcessor.h"

#include "BuildCommon/ImportModel.h"
#include "BuildCommon/BuildScene.h"
#include "BuildCommon/BakeScene.h"
#include "BuildCore/BuildContext.h"
#include "SceneLib/SceneResource.h"
#include "Assets/AssetFileUtils.h"
#include "SystemLib/MemoryAllocation.h"

namespace Selas
{
    //=============================================================================================================================
    Error CModelBuildProcessor::Setup()
    {
        AssetFileUtils::EnsureAssetDirectory<SceneResource>();
        AssetFileUtils::EnsureAssetDirectory(SceneResource::kGeometryDataType, SceneResource::kDataVersion);

        return Success_;
    }

    //=============================================================================================================================
    cpointer CModelBuildProcessor::Type()
    {
        return "model";
    }

    //=============================================================================================================================
    uint64 CModelBuildProcessor::Version()
    {
        return SceneResource::kDataVersion;
    }

    //=============================================================================================================================
    Error CModelBuildProcessor::Process(BuildProcessorContext* context)
    {
        ImportedModel importedModel;
        ReturnError_(ImportModel(context, &importedModel));

        //cpointer materialprefix = "Scenes~SanMiguel~Materials~";
        cpointer materialprefix = "Materials~";

        BuiltScene builtScene;
        ReturnError_(BuildScene(context, materialprefix, &importedModel, &builtScene));
        ShutdownImportedModel(&importedModel);

        for(uint scan = 0, count = builtScene.textures.Length(); scan < count; ++scan) {
            cpointer textureName = builtScene.textures[scan].Ascii();
            context->AddProcessDependency("Texture", textureName);
        }

        BakeScene(context, builtScene);

        return Success_;
    }
}