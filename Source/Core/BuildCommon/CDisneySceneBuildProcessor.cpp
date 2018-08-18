//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/CDisneySceneBuildProcessor.h"

#include "BuildCommon/ImportModel.h"
#include "BuildCommon/BuildScene.h"
#include "BuildCommon/BakeScene.h"
#include "BuildCore/BuildContext.h"
#include "UtilityLib/JsonUtilities.h"
#include "SceneLib/SceneResource.h"
#include "Assets/AssetFileUtils.h"
#include "SystemLib/MemoryAllocation.h"

namespace Selas
{
    //=============================================================================================================================
    struct SceneFileData
    {
        CArray<FixedString256> elements;
    };

    //=============================================================================================================================
    struct SElementData
    {
        FilePathString objSourceId;
        FilePathString materialFile;
        float4x4 transform;
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
            FixedString256& str = output.elements.Add();
            str.Copy(element.GetString());
        }

        return Success_;
    }

    //=============================================================================================================================
    static Error ParseElementFile(BuildProcessorContext* context, const FixedString256& root, const FixedString256& path,
                                  SElementData& elementData)
    {
        FilePathString filepath;
        AssetFileUtils::ContentFilePath(path.Ascii(), filepath);
        context->AddFileDependency(filepath.Ascii());

        rapidjson::Document document;
        ReturnError_(Json::OpenJsonDocument(filepath.Ascii(), document));

        cpointer objFile = document["geomObjFile"].GetString();
        cpointer materialFile = document["matFile"].GetString();

        FixedStringSprintf(elementData.objSourceId, "%s%s", root.Ascii(), objFile);
        StringUtil::ReplaceAll(elementData.objSourceId.Ascii(), '\\', PlatformIndependentPathSep_);
        StringUtil::ReplaceAll(elementData.objSourceId.Ascii(), '/', PlatformIndependentPathSep_);

        AssetFileUtils::ContentFilePath(root.Ascii(), materialFile, "", elementData.materialFile);

        return Success_;
    }

    //=============================================================================================================================
    Error CDisneySceneBuildProcessor::Setup()
    {
        AssetFileUtils::EnsureAssetDirectory<SceneResource>();
        AssetFileUtils::EnsureAssetDirectory(SceneResource::kGeometryDataType, SceneResource::kDataVersion);

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

        for(uint scan = 0, count = sceneFile.elements.Count(); scan < count; ++scan) {
            const FixedString256& elementName = sceneFile.elements[scan];

            SElementData elementData;
            ReturnError_(ParseElementFile(context, contentRoot, elementName, elementData));
        }

        return Success_;
    }
}