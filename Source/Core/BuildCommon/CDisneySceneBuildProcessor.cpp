//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/CDisneySceneBuildProcessor.h"

#include "BuildCommon/ImportModel.h"
#include "BuildCommon/ImportMaterial.h"
#include "BuildCore/BuildContext.h"
#include "UtilityLib/JsonUtilities.h"
#include "SceneLib/SceneResource.h"
#include "Assets/AssetFileUtils.h"
#include "MathLib/FloatFuncs.h"
#include "SystemLib/Logging.h"
#include "SystemLib/MemoryAllocation.h"

namespace Selas
{
    //=============================================================================================================================
    struct SceneFileData
    {
        FilePathString cameraFile;
        CArray<FilePathString> elements;
    };

    //=============================================================================================================================
    static FixedString256 ContentRoot(BuildProcessorContext* context)
    {
        FixedString256 root;
        root.Copy(context->source.name.Ascii());

        char* addr = StringUtil::FindLastChar(root.Ascii(), '~');
        if(addr == nullptr) {
            root.Clear();
        }
        else {
            *(addr + 1) = '\0';
        }
        
        return root;
    }

    //=============================================================================================================================
    static Error ParseInstancePrimitivesFile(BuildProcessorContext* context, const FixedString256& root,
                                             const FilePathString& sourceId, SceneResourceData* scene)
    {
        FilePathString filepath;
        AssetFileUtils::ContentFilePath(sourceId.Ascii(), filepath);
        ReturnError_(context->AddFileDependency(filepath.Ascii()));

        rapidjson::Document document;
        ReturnError_(Json::OpenJsonDocument(filepath.Ascii(), document));

        for(const auto& objFileKV : document.GetObject()) {
            cpointer objFile = objFileKV.name.GetString();

            FilePathString instanceObjFile;
            FixedStringSprintf(instanceObjFile, "%s%s", root.Ascii(), objFile);
            AssetFileUtils::IndependentPathSeperators(instanceObjFile);

            uint meshIndex = scene->modelNames.Add(instanceObjFile);

            const auto& element = objFileKV.value;
            scene->modelInstances.Reserve(scene->modelInstances.Count() + element.GetObject().MemberCount());
            for(const auto& instanceKV : element.GetObject()) {
                cpointer instanceName = instanceKV.name.GetString();

                Instance modelInstance;
                modelInstance.index = meshIndex;

                if(Json::ReadMatrix4x4(instanceKV.value, modelInstance.localToWorld) == false) {
                    return Error_("Failed to parse transform from instance '%s' in primfile '%s'", instanceName, filepath.Ascii());
                }
                modelInstance.worldToLocal = MatrixInverse(modelInstance.localToWorld);
                
                scene->modelInstances.Add(modelInstance);
            }
        }

        return Success_;
    }

    //=============================================================================================================================
    static Error ParseInstancePrimitivesSection(BuildProcessorContext* context, const FixedString256& root,
                                                const rapidjson::Value& section, SceneResourceData* scene)
    {
        for(const auto& keyvalue : section.GetObject()) {
            const auto& element = keyvalue.value;

            FixedString32 type;
            if(Json::ReadFixedString(element, "type", type) == false) {
                return Error_("'type' parameter missing from instanced primitives section.");
            }

            if(StringUtil::Equals(type.Ascii(), "archive") == false) {
                WriteDebugInfo_(type.Ascii());
                continue;
            }

            FilePathString primFile;
            if(Json::ReadFixedString(element, "jsonFile", primFile) == false) {
                return Error_("`jsonFile ` parameter missing from instanced primitives section");
            }
            AssetFileUtils::IndependentPathSeperators(primFile);
            
            FilePathString sourceId;
            FixedStringSprintf(sourceId, "%s%s", root.Ascii(), primFile.Ascii());

            ReturnError_(ParseInstancePrimitivesFile(context, root, sourceId, scene));
        }

        return Success_;
    }

    //=============================================================================================================================
    static Error ImportDisneyMaterials(BuildProcessorContext* context, const FilePathString& contentId, cpointer prefix,
                                       ImportedModel* imported)
    {
        FilePathString filepath;
        AssetFileUtils::ContentFilePath(contentId.Ascii(), filepath);
        ReturnError_(context->AddFileDependency(filepath.Ascii()));

        CArray<Hash32> materialHashes;
        CArray<ImportedMaterialData> importedMaterials;
        ReturnError_(ImportDisneyMaterials(filepath.Ascii(), prefix, materialHashes, importedMaterials));

        //imported->loadedMaterialHashes.Append(materialHashes);
        //imported->loadedMaterials.Append(importedMaterials);

        return Success_;
    }

    //=============================================================================================================================
    static Error ParseSceneFile(BuildProcessorContext* context, SceneFileData& output)
    {
        FilePathString filepath;
        AssetFileUtils::ContentFilePath(context->source.name.Ascii(), filepath);
        context->AddFileDependency(filepath.Ascii());

        rapidjson::Document document;
        ReturnError_(Json::OpenJsonDocument(filepath.Ascii(), document));

        if(document.HasMember("elements") == false) {
            return Error_("'elements' member not found in Disney scene '%s'", filepath.Ascii());
        }

        for(const auto& element : document["elements"].GetArray()) {
            FilePathString& str = output.elements.Add();
            str.Copy(element.GetString());
        }

        Json::ReadFixedString(document, "camera", output.cameraFile);

        return Success_;
    }

    //=============================================================================================================================
    static Error ParseElementFile(BuildProcessorContext* context, const FixedString256& root, const FilePathString& path,
                                  SceneResourceData* rootScene, CArray<SceneResourceData*>& scenes)
    {
        FilePathString elementFilePath;
        AssetFileUtils::ContentFilePath(path.Ascii(), elementFilePath);
        ReturnError_(context->AddFileDependency(elementFilePath.Ascii()));

        rapidjson::Document document;
        ReturnError_(Json::OpenJsonDocument(elementFilePath.Ascii(), document));

        // -- Get the materials json file path
        //FixedStringSprintf(mesh->materialFile, "%s%s", root.Ascii(), document["matFile"].GetString());

        // -- Add the main geometry object file
        FilePathString geomObjFile;
        FixedStringSprintf(geomObjFile, "%s%s", root.Ascii(), document["geomObjFile"].GetString());
        AssetFileUtils::IndependentPathSeperators(geomObjFile);
        uint rootModelIndex = rootScene->modelNames.Add(geomObjFile);

        uint childSceneIndex = InvalidIndex64;

        // -- create a child scene to contain the instanced primitives
        if(document.HasMember("instancedPrimitiveJsonFiles")) {
            SceneResourceData* primitivesScene = New_(SceneResourceData);
            primitivesScene->name.Copy(path.Ascii());
            primitivesScene->iblName.Clear();
            primitivesScene->backgroundIntensity = float4::Zero_;
            InvalidCameraSettings(&primitivesScene->camera);
            
            childSceneIndex = rootScene->sceneNames.Add(primitivesScene->name);

            // -- read the instanced primitives section.
            ReturnError_(ParseInstancePrimitivesSection(context, root, document["instancedPrimitiveJsonFiles"], primitivesScene));

            scenes.Add(primitivesScene);
        }

        {
            // -- Each element file will have a transform for the 'root level' object file...
            Instance rootInstance;
            Json::ReadMatrix4x4(document["transformMatrix"], rootInstance.localToWorld);
            rootInstance.worldToLocal = MatrixInverse(rootInstance.localToWorld);
            rootInstance.index = rootModelIndex;
            rootScene->modelInstances.Add(rootInstance);

            if(childSceneIndex != InvalidIndex64) {
                rootInstance.index = childSceneIndex;
                rootScene->sceneInstances.Add(rootInstance);
            }
        }

        // -- add instanced copies
        if(document.HasMember("instancedCopies")) {
            for(const auto& instancedCopyKV : document["instancedCopies"].GetObject()) {

                Instance copyInstance;
                if(Json::ReadMatrix4x4(instancedCopyKV.value["transformMatrix"], copyInstance.localToWorld) == false) {
                    return Error_("Failed to read `transformMatrix` from instancedCopy '%s'", instancedCopyKV.name.GetString());
                }
                copyInstance.worldToLocal = MatrixInverse(copyInstance.localToWorld);

                uint modelIndex = rootModelIndex;
                if(instancedCopyKV.value.HasMember("geomObjFile")) {
                    FilePathString altGeomObjFile;
                    FixedStringSprintf(altGeomObjFile, "%s%s", root.Ascii(), instancedCopyKV.value["geomObjFile"].GetString());
                    AssetFileUtils::IndependentPathSeperators(altGeomObjFile);
                    modelIndex = rootScene->modelNames.Add(altGeomObjFile);
                }

                copyInstance.index = modelIndex;
                rootScene->modelInstances.Add(copyInstance);

                uint sceneIndex = childSceneIndex;

                if(instancedCopyKV.value.HasMember("instancedPrimitiveJsonFiles")) {
                    SceneResourceData* altScene = New_(SceneResourceData);
                    altScene->name.Copy(instancedCopyKV.name.GetString());

                    altScene->iblName.Clear();
                    altScene->backgroundIntensity = float4::Zero_;
                    InvalidCameraSettings(&altScene->camera);

                    sceneIndex = rootScene->sceneNames.Add(altScene->name);

                    // -- read the instanced primitives section.
                    ReturnError_(ParseInstancePrimitivesSection(context, root, instancedCopyKV.value["instancedPrimitiveJsonFiles"], altScene));

                    scenes.Add(altScene);
                }

                if(sceneIndex != InvalidIndex64) {
                    copyInstance.index = sceneIndex;
                    rootScene->sceneInstances.Add(copyInstance);
                }
            }
        }

        return Success_;
    }

    //=============================================================================================================================
    static Error ParseCameraFile(BuildProcessorContext* context, cpointer path, CameraSettings& settings)
    {
        FilePathString fullPath;
        AssetFileUtils::ContentFilePath(path, fullPath);
        ReturnError_(context->AddFileDependency(fullPath.Ascii()));

        rapidjson::Document document;
        ReturnError_(Json::OpenJsonDocument(fullPath.Ascii(), document));

        Json::ReadFloat3(document, "eye", settings.position, float3(1.0f, 0.0f, 0.0f));
        Json::ReadFloat(document, "fov", settings.fov, 70.0f);
        Json::ReadFloat3(document, "look", settings.lookAt, float3::Zero_);
        Json::ReadFloat3(document, "up", settings.up, float3::YAxis_);
        
        settings.fov *= Math::DegreesToRadians_;
        settings.znear = 0.1f;
        settings.zfar = 50000.0f;

        return Success_;
    }

    //=============================================================================================================================
    Error CDisneySceneBuildProcessor::Setup()
    {
        AssetFileUtils::EnsureAssetDirectory<SceneResource>();

        return Success_;
    }

    //=============================================================================================================================
    cpointer CDisneySceneBuildProcessor::Type()
    {
        return "disney";
    }

    //=============================================================================================================================
    uint64 CDisneySceneBuildProcessor::Version()
    {
        return SceneResource::kDataVersion;
    }

    //=============================================================================================================================
    Error CDisneySceneBuildProcessor::Process(BuildProcessorContext* context)
    {
        FixedString256 contentRoot = ContentRoot(context);

        SceneFileData sceneFile;
        ReturnError_(ParseSceneFile(context, sceneFile));

        CArray<SceneResourceData*> allScenes;

        SceneResourceData* rootScene = New_(SceneResourceData);
        rootScene->name.Copy(context->source.name.Ascii());
        rootScene->backgroundIntensity = float4::One_;
        allScenes.Add(rootScene);

        ReturnError_(ParseCameraFile(context, sceneFile.cameraFile.Ascii(), rootScene->camera));
        
        for(uint scan = 0, count = sceneFile.elements.Count(); scan < count; ++scan) {
            const FilePathString& elementName = sceneFile.elements[scan];
            ReturnError_(ParseElementFile(context, contentRoot, elementName, rootScene, allScenes));
        }

        for(uint scan = 0, count = allScenes.Count(); scan < count; ++scan) {

            SceneResourceData* scene = allScenes[scan];
            for(uint scan = 0, count = scene->modelNames.Count(); scan < count; ++scan) {
                context->AddProcessDependency("model", scene->modelNames[scan].Ascii());
            }

            context->CreateOutput(SceneResource::kDataType, SceneResource::kDataVersion, scene->name.Ascii(), *scene);
            Delete_(scene);
        }

        allScenes.Shutdown();

        return Success_;
    }
}