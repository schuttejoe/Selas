//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/CSceneBuildProcessor.h"
#include "BuildCore/BuildContext.h"
#include "SceneLib/SceneResource.h"
#include "SceneLib/SubsceneResource.h"
#include "UtilityLib/JsonUtilities.h"
#include "Assets/AssetFileUtils.h"
#include "MathLib/FloatFuncs.h"
#include "SystemLib/MemoryAllocation.h"

namespace Selas
{
    struct InstanceDescription
    {
        FilePathString asset;
        float4x4 localToWorld;
        float4x4 worldToLocal;
    };

    struct SceneDescription
    {
        FilePathString iblName;
        float3 backgroundIntensity;
        CArray<InstanceDescription> modelInstances;
        CArray<InstanceDescription> sceneInstances;
    };

    //=============================================================================================================================
    static Error ParseSceneFile(BuildProcessorContext* context, SceneDescription* scene)
    {
        FilePathString filepath;
        AssetFileUtils::ContentFilePath(context->source.name.Ascii(), filepath);
        ReturnError_(context->AddFileDependency(filepath.Ascii()));

        rapidjson::Document document;
        ReturnError_(Json::OpenJsonDocument(filepath.Ascii(), document));

        if(document.HasMember("instances")) {
            for(const auto& instance : document["instances"].GetArray()) {
                InstanceDescription& desc = scene->modelInstances.Add();
                Json::ReadFixedString(instance, "model", desc.asset);
                Json::ReadMatrix4x4(instance, "transform", desc.localToWorld);
                desc.worldToLocal = MatrixInverse(desc.localToWorld);
            }
        }

        if(document.HasMember("scenes")) {
            for(const auto& instance : document["scenes"].GetArray()) {
                InstanceDescription& desc = scene->sceneInstances.Add();
                Json::ReadFixedString(instance, "scene", desc.asset);
                Json::ReadMatrix4x4(instance, "transform", desc.localToWorld);
                desc.worldToLocal = MatrixInverse(desc.localToWorld);
            }
        }

        Json::ReadFloat3(document, "backgroundIntensity", scene->backgroundIntensity, float3::One_);
        Json::ReadFixedString(document, "ibl", scene->iblName);

        return Success_;
    }

    //=============================================================================================================================
    Error CSceneBuildProcessor::Setup()
    {
        AssetFileUtils::EnsureAssetDirectory<SceneResource>();

        return Success_;
    }

    //=============================================================================================================================
    cpointer CSceneBuildProcessor::Type()
    {
        return "scene";
    }

    //=============================================================================================================================
    uint64 CSceneBuildProcessor::Version()
    {
        return SceneResource::kDataVersion;
    }

    //=============================================================================================================================
    Error CSceneBuildProcessor::Process(BuildProcessorContext* context)
    {
        //FixedString256 contentRoot = ContentRoot(context);

        SceneDescription sceneDesc;
        ReturnError_(ParseSceneFile(context, &sceneDesc));

        CSet<Hash32> modelHashes;
        CSet<Hash32> sceneHashes;

        SceneResourceData scene;
        SubsceneResourceData subscene;

        scene.backgroundIntensity = float4(sceneDesc.backgroundIntensity, 1.0f);
        scene.iblName = sceneDesc.iblName;

        if(StringUtil::Length(scene.iblName.Ascii()) > 0) {
            context->AddProcessDependency("HDR", scene.iblName.Ascii());
        }

        FilePathString sceneName;
        sceneName.Copy(context->source.name.Ascii());


        uint subSceneIndex = scene.subsceneNames.Add(sceneName);
        Instance& instance = scene.subsceneInstances.Add();
        instance.index = subSceneIndex;
        instance.localToWorld = Matrix4x4::Identity();

        for(uint scan = 0, count = sceneDesc.modelInstances.Count(); scan < count; ++scan) {
            InstanceDescription& instanceDesc = sceneDesc.modelInstances[scan];

            Hash32 hash = MurmurHash3_x86_32(instanceDesc.asset.Ascii(), StringUtil::Length(instanceDesc.asset.Ascii()));

            uint64 modelindex = modelHashes.Add(hash);
            if(modelindex >= subscene.modelNames.Count()) {
                context->AddProcessDependency("model", instanceDesc.asset.Ascii());
                subscene.modelNames.Add(instanceDesc.asset);
            }

            Instance& instance = subscene.modelInstances.Add();
            instance.index = modelindex;
            instance.localToWorld = instanceDesc.localToWorld;
        }

        for(uint scan = 0, count = sceneDesc.sceneInstances.Count(); scan < count; ++scan) {
            InstanceDescription& instanceDesc = sceneDesc.sceneInstances[scan];

            Hash32 hash = MurmurHash3_x86_32(instanceDesc.asset.Ascii(), StringUtil::Length(instanceDesc.asset.Ascii()));

            uint64 sceneIndex = sceneHashes.Add(hash);
            if(sceneIndex >= scene.subsceneNames.Count()) {
                context->AddProcessDependency("scene", instanceDesc.asset.Ascii());
                scene.subsceneNames.Add(instanceDesc.asset);
            }

            Instance& instance = scene.subsceneInstances.Add();
            instance.index = sceneIndex;
            instance.localToWorld = instanceDesc.localToWorld;
        }

        context->CreateOutput(SceneResource::kDataType, SceneResource::kDataVersion, context->source.name.Ascii(), scene);
        context->CreateOutput(SubsceneResource::kDataType, SubsceneResource::kDataVersion, context->source.name.Ascii(), subscene);

        return Success_;
    }
}