//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/CModelBuildProcessor.h"

#include "BuildCommon/ImportModel.h"
#include "BuildCommon/BuildModel.h"
#include "BuildCommon/BakeModel.h"
#include "BuildCore/BuildContext.h"
#include "SceneLib/ModelResource.h"
#include "Assets/AssetFileUtils.h"
#include "SystemLib/MemoryAllocation.h"

namespace Selas
{
    //=============================================================================================================================
    Error CModelBuildProcessor::Setup()
    {
        AssetFileUtils::EnsureAssetDirectory<ModelResource>();
        AssetFileUtils::EnsureAssetDirectory(ModelResource::kGeometryDataType, ModelResource::kDataVersion);

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
        return ModelResource::kDataVersion;
    }

    //=============================================================================================================================
    Error CModelBuildProcessor::Process(BuildProcessorContext* context)
    {
        ImportedModel importedModel;
        ReturnError_(ImportModel(context, &importedModel));

        //cpointer materialprefix = "Scenes~SanMiguel~Materials~";
        cpointer materialprefix = "Materials~";

        BuiltModel builtScene;
        ReturnError_(BuildScene(context, materialprefix, &importedModel, &builtScene));
        ShutdownImportedModel(&importedModel);

        for(uint scan = 0, count = builtScene.textures.Count(); scan < count; ++scan) {
            cpointer textureName = builtScene.textures[scan].Ascii();
            context->AddProcessDependency("Texture", textureName);
        }

        BakeModel(context, builtScene);

        return Success_;
    }
}