//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/CDisneySceneBuildProcessor.h"

#include "BuildCommon/ImportMaterial.h"
#include "BuildCommon/ModelBuildPipeline.h"
#include "BuildCommon/BakeModel.h"
#include "BuildCommon/BuildModel.h"
#include "BuildCore/BuildContext.h"
#include "SceneLib/SceneResource.h"
#include "SceneLib/SubSceneResource.h"
#include "Assets/AssetFileUtils.h"
#include "UtilityLib/JsonUtilities.h"
#include "UtilityLib/MurmurHash.h"
#include "IoLib/Directory.h"
#include "MathLib/FloatFuncs.h"
#include "SystemLib/Logging.h"
#include "SystemLib/MemoryAllocation.h"
#include "SystemLib/CheckedCast.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

namespace Selas
{
    #define CurveModelNameSuffix_ "_generatedcurves"

    //=============================================================================================================================
    struct SceneFileData
    {
        FilePathString cameraFile;
        FilePathString iblFile;
        FilePathString lightsFile;
        CArray<FilePathString> elements;
    };

    //=============================================================================================================================
    struct CurveSegment
    {
        uint64 startIndex;
        uint64 controlPointCount;
    };

    //=============================================================================================================================
    struct CurveData
    {
        float widthTip;
        float widthRoot;
        float degrees;
        bool  faceCamera;
        CArray<float3> controlPoints;
        CArray<CurveSegment> segments;
        Hash32 name;
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
    static Error ParseArchiveFile(BuildProcessorContext* context, const FixedString256& root,
                                  const FilePathString& sourceId, SubsceneResourceData* subscene)
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

            uint meshIndex = subscene->modelNames.Add(instanceObjFile);

            const auto& element = objFileKV.value;
            subscene->modelInstances.Reserve(subscene->modelInstances.Count() + element.GetObject().MemberCount());
            for(const auto& instanceKV : element.GetObject()) {
                cpointer instanceName = instanceKV.name.GetString();

                Instance modelInstance;
                modelInstance.index = meshIndex;

                if(Json::ReadMatrix4x4(instanceKV.value, modelInstance.localToWorld) == false) {
                    return Error_("Failed to parse transform from instance '%s' in primfile '%s'", instanceName, filepath.Ascii());
                }
                modelInstance.worldToLocal = MatrixInverse(modelInstance.localToWorld);
                
                subscene->modelInstances.Add(modelInstance);
            }
        }

        return Success_;
    }

    //=============================================================================================================================
    static Error ParseControlPointsFile(BuildProcessorContext* context, cpointer path, CurveData* importedCurve)
    {
        FilePathString filepath;
        AssetFileUtils::ContentFilePath(path, filepath);
        ReturnError_(context->AddFileDependency(filepath.Ascii()));

        rapidjson::Document document;
        ReturnError_(Json::OpenJsonDocument(filepath.Ascii(), document));

        uint totalControlPoints = 0;

        // -- Do a prepass to count the total number of control points
        uint segmentCount = document.Size();
        for(const auto& curveElement : document.GetArray()) {
            uint controlPointCount = curveElement.Size();
            totalControlPoints += controlPointCount;
        }

        importedCurve->controlPoints.Resize(totalControlPoints);
        importedCurve->segments.Resize(segmentCount);

        uint segmentIndex = 0;
        uint cpIndex = 0;

        for(const auto& curveElement : document.GetArray()) {
            uint controlPointCount = curveElement.Size();

            importedCurve->segments[segmentIndex].startIndex = cpIndex;
            importedCurve->segments[segmentIndex].controlPointCount = controlPointCount;
            ++segmentIndex;

            for(const auto& controlPointElement : curveElement.GetArray()) {
                float3& controlPoint = importedCurve->controlPoints[cpIndex];
                ++cpIndex;

                controlPoint.x = controlPointElement[0].GetFloat();
                controlPoint.y = controlPointElement[1].GetFloat();
                controlPoint.z = controlPointElement[2].GetFloat();
            }
        }

        return Success_;
    }

    //=============================================================================================================================
    static void BuildCurveModel(CurveData* importedCurve, cpointer curveName, BuiltModel& curveModel)
    {
        uint controlPointCount = importedCurve->controlPoints.Count();
        uint curveCount = importedCurve->segments.Count();

        MakeInvalid(&curveModel.aaBox);

        uint curveIndex = 0;
        uint64 indexOffset = 0;

        curveModel.curveModelNameHash = MurmurHash3_x86_32(curveName, StringUtil::Length(curveName));
        curveModel.curves.Resize(curveCount);
        curveModel.curveIndices.Reserve(controlPointCount + curveCount * 3);
        curveModel.curveVertices.Reserve(controlPointCount + curveCount * 2);

        float widthRoot = importedCurve->widthRoot;
        float widthTip = importedCurve->widthTip;

        for(uint segIndex = 0, segCount = importedCurve->segments.Count(); segIndex < segCount; ++segIndex) {

            const CurveSegment& segment = importedCurve->segments[segIndex];

            CurveMetaData& curve = curveModel.curves[curveIndex++];
            curve.indexOffset = CheckedCast<uint32>(curveModel.curveIndices.Count());
            curve.indexCount = CheckedCast<uint32>(segment.controlPointCount - 1);

            for(uint cpScan = 0; cpScan < segment.controlPointCount - 1; ++cpScan) {
                curveModel.curveIndices.Add(CheckedCast<uint32>(indexOffset));
                ++indexOffset;
            }
            indexOffset += 3;

            curveModel.curveVertices.Add(float4(importedCurve->controlPoints[segment.startIndex], widthRoot));
            for(uint cpScan = 0; cpScan < segment.controlPointCount; ++cpScan) {

                float w = Lerp(widthRoot, widthTip, (float)cpScan / (float)(segment.controlPointCount - 1));
                float3 cpPosition = importedCurve->controlPoints[segment.startIndex + cpScan];
                IncludePosition(&curveModel.aaBox, cpPosition);

                curveModel.curveVertices.Add(float4(cpPosition, w));
            }
            curveModel.curveVertices.Add(float4(importedCurve->controlPoints[segment.startIndex + segment.controlPointCount - 1], widthTip));
        }
    }

    //=============================================================================================================================
    static Error ParseCurveElement(BuildProcessorContext* context, cpointer curveName, const FixedString256& root,
                                   const rapidjson::Value& element, SubsceneResourceData* subscene)
    {
        FilePathString curveFile;
        if(Json::ReadFixedString(element, "jsonFile", curveFile) == false) {
            return Error_("`jsonFile ` parameter missing from instanced primitives section");
        }
        AssetFileUtils::IndependentPathSeperators(curveFile);

        CurveData curve;
        Json::ReadFloat(element, "widthTip", curve.widthTip, 1.0f);
        Json::ReadFloat(element, "widthRoot", curve.widthRoot, 1.0f);
        Json::ReadFloat(element, "degrees", curve.degrees, 1.0f);
        Json::ReadBool(element, "faceCamera", curve.faceCamera, false);

        FilePathString controlPointsFile;
        FixedStringSprintf(controlPointsFile, "%s%s", root.Ascii(), curveFile.Ascii());

        FilePathString curveModelName;
        FixedStringSprintf(curveModelName, "%s%s%s", root.Ascii(), curveFile.Ascii(), CurveModelNameSuffix_);
       
        ReturnError_(ParseControlPointsFile(context, controlPointsFile.Ascii(), &curve));
        
        BuiltModel curveModel;
        BuildCurveModel(&curve, curveName, curveModel);

        ReturnError_(BakeModel(context, curveModelName.Ascii(), curveModel));

        Instance curveInstance;
        curveInstance.index = subscene->modelNames.Add(curveModelName);
        curveInstance.localToWorld = Matrix4x4::Identity();
        curveInstance.worldToLocal = Matrix4x4::Identity();
        subscene->modelInstances.Add(curveInstance);

        return Success_;
    }

    //=============================================================================================================================
    static Error ParseInstancePrimitivesSection(BuildProcessorContext* context, const FixedString256& root,
                                                const rapidjson::Value& section, SubsceneResourceData* subscene)
    {
        uint controlPointCount = 0;
        uint curveCount = 0;

        for(const auto& keyvalue : section.GetObject()) {
            const auto& element = keyvalue.value;

            FixedString32 type;
            if(Json::ReadFixedString(element, "type", type) == false) {
                return Error_("'type' parameter missing from instanced primitives section.");
            }

            if(StringUtil::Equals(type.Ascii(), "archive")) {
                FilePathString primFile;
                if(Json::ReadFixedString(element, "jsonFile", primFile) == false) {
                    return Error_("`jsonFile ` parameter missing from instanced primitives section");
                }
                AssetFileUtils::IndependentPathSeperators(primFile);

                FilePathString sourceId;
                FixedStringSprintf(sourceId, "%s%s", root.Ascii(), primFile.Ascii());
                ReturnError_(ParseArchiveFile(context, root, sourceId, subscene));
            }
            else if(StringUtil::Equals(type.Ascii(), "curve")) {
                ReturnError_(ParseCurveElement(context, keyvalue.name.GetString(), root, element, subscene));
            }
        }

        return Success_;
    }

    //=============================================================================================================================
    static Error ParseLightsFile(BuildProcessorContext* context, cpointer lightfile, SceneResourceData* scene)
    {
        if(StringUtil::Length(lightfile) == 0) {
            return Success_;
        }

        FilePathString filepath;
        AssetFileUtils::ContentFilePath(lightfile, filepath);
        ReturnError_(context->AddFileDependency(filepath.Ascii()));

        rapidjson::Document document;
        ReturnError_(Json::OpenJsonDocument(filepath.Ascii(), document));

        uint lightCount = document.MemberCount();
        scene->lights.Reserve(lightCount);

        for(const auto& lightElementKV : document.GetObject()) {
            
            FixedString64 type;
            Json::ReadFixedString(lightElementKV.value, "type", type);
            if(StringUtil::Equals(type.Ascii(), "quad") == false) {
                continue;
            }

            float4 color;
            float exposure;
            Json::ReadFloat4(lightElementKV.value, "color", color, float4::Zero_);
            Json::ReadFloat(lightElementKV.value, "exposure", exposure, 0.0f);

            float width, height;
            Json::ReadFloat(lightElementKV.value, "width", width, 0.0f);
            Json::ReadFloat(lightElementKV.value, "height", height, 0.0f);

            float3 rgb = color.XYZ();
            float3 radiance = Math::Powf(2.0f, exposure) * Pow(rgb, 2.2f);
            if(Dot(radiance, float3::One_) <= 0.0f) {
                continue;
            }

            SceneLight& light = scene->lights.Add();

            float4x4 matrix;
            Json::ReadMatrix4x4(lightElementKV.value, "translationMatrix", matrix);

            light.position = MatrixMultiplyPoint(float3::Zero_, matrix);
            light.direction = MatrixMultiplyVector(-float3::ZAxis_, matrix);
            light.x = MatrixMultiplyVector(width * float3::XAxis_, matrix);
            light.z = MatrixMultiplyVector(height * float3::YAxis_, matrix);

            light.radiance = radiance;
            light.type = QuadLight;
        }

        return Success_;
    }

    //=============================================================================================================================
    static Error ImportDisneyMaterials(BuildProcessorContext* context, cpointer materialPath, cpointer prefix,
                                       SubsceneResourceData* subscene)
    {
        FilePathString filepath;
        AssetFileUtils::ContentFilePath(materialPath, filepath);
        ReturnError_(context->AddFileDependency(filepath.Ascii()));

        CArray<FixedString64> materialNames;
        CArray<ImportedMaterialData> importedMaterials;
        ReturnError_(ImportDisneyMaterials(filepath.Ascii(), materialNames, importedMaterials));

        subscene->sceneMaterialNames.Reserve(importedMaterials.Count());
        subscene->sceneMaterials.Reserve(importedMaterials.Count());
        for(uint scan = 0, count = importedMaterials.Count(); scan < count; ++scan) {
            
            if(importedMaterials[scan].baseColorTexture.Length() > 0) {
                FilePathString temp;
                temp.Copy(importedMaterials[scan].baseColorTexture.Ascii());
                FixedStringSprintf(importedMaterials[scan].baseColorTexture, "%s%s", prefix, temp.Ascii());
            }

            Hash32 materialNameHash = MurmurHash3_x86_32(materialNames[scan].Ascii(), (int32)materialNames[scan].Length());
            subscene->sceneMaterialNames.Add(materialNameHash);

            MaterialResourceData& material = subscene->sceneMaterials.Add();
            BuildMaterial(importedMaterials[scan], material);
        }

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
        Json::ReadFixedString(document, "ibl", output.iblFile);
        Json::ReadFixedString(document, "lights", output.lightsFile);

        return Success_;
    }

    //=============================================================================================================================
    static Error AllocateSubscene(BuildProcessorContext* context, CArray<SubsceneResourceData*>& scenes,
                                  cpointer name, const FilePathString& materialFile, const FixedString256& root,
                                  SubsceneResourceData*& scene)
    {
        scene = New_(SubsceneResourceData);
        scene->name.Copy(name);

        ReturnError_(ImportDisneyMaterials(context, materialFile.Ascii(), root.Ascii(), scene));

        scenes.Add(scene);
        return Success_;
    }

    //=============================================================================================================================
    static Error ParseElementFile(BuildProcessorContext* context, const FixedString256& root, const FilePathString& path,
                                  SceneResourceData* rootScene, CArray<SubsceneResourceData*>& scenes)
    {
        FilePathString elementFilePath;
        AssetFileUtils::ContentFilePath(path.Ascii(), elementFilePath);
        ReturnError_(context->AddFileDependency(elementFilePath.Ascii()));

        rapidjson::Document document;
        ReturnError_(Json::OpenJsonDocument(elementFilePath.Ascii(), document));

        // -- Prepare the materials json file path
        FilePathString materialFile;
        FixedStringSprintf(materialFile, "%s%s", root.Ascii(), document["matFile"].GetString());

        FilePathString geomSceneName;
        FixedStringSprintf(geomSceneName, "%s_geometry", path.Ascii());

        SubsceneResourceData* elementGeometryScene;
        ReturnError_(AllocateSubscene(context, scenes, geomSceneName.Ascii(), materialFile, root, elementGeometryScene));
        uint geometrySceneIndex = rootScene->subsceneNames.Add(geomSceneName);
        
        Instance geometrySceneInstance;
        geometrySceneInstance.index = geometrySceneIndex;
        rootScene->subsceneInstances.Add(geometrySceneInstance);

        // -- Add the main geometry file
        FilePathString geomObjFile;
        FixedStringSprintf(geomObjFile, "%s%s", root.Ascii(), document["geomObjFile"].GetString());
        AssetFileUtils::IndependentPathSeperators(geomObjFile);
        uint rootModelIndex = elementGeometryScene->modelNames.Add(geomObjFile);

        // -- create a child scene to contain the instanced primitives
        uint primitivesSceneIndex = InvalidIndex64;
        if(document.HasMember("instancedPrimitiveJsonFiles")) {
            SubsceneResourceData* primitivesScene;
            ReturnError_(AllocateSubscene(context, scenes, path.Ascii(), materialFile, root, primitivesScene));
            primitivesSceneIndex = rootScene->subsceneNames.Add(primitivesScene->name);

            // -- read the instanced primitives section.
            ReturnError_(ParseInstancePrimitivesSection(context, root, document["instancedPrimitiveJsonFiles"], primitivesScene));
        }

        {
            // -- Each element file will have a transform for the 'root level' object file...
            Instance rootInstance;
            Json::ReadMatrix4x4(document["transformMatrix"], rootInstance.localToWorld);
            rootInstance.worldToLocal = MatrixInverse(rootInstance.localToWorld);
            rootInstance.index = rootModelIndex;
            elementGeometryScene->modelInstances.Add(rootInstance);

            if(primitivesSceneIndex != InvalidIndex64) {
                rootInstance.index = primitivesSceneIndex;
                rootScene->subsceneInstances.Add(rootInstance);
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
                    modelIndex = elementGeometryScene->modelNames.Add(altGeomObjFile);
                }

                copyInstance.index = modelIndex;
                elementGeometryScene->modelInstances.Add(copyInstance);

                uint sceneIndex = primitivesSceneIndex;

                if(instancedCopyKV.value.HasMember("instancedPrimitiveJsonFiles")
                   && instancedCopyKV.value["instancedPrimitiveJsonFiles"].MemberCount() > 0) {

                    SubsceneResourceData* altScene;
                    ReturnError_(AllocateSubscene(context, scenes, instancedCopyKV.name.GetString(), materialFile, root, altScene));
                    sceneIndex = rootScene->subsceneNames.Add(altScene->name);

                    // -- read the instanced primitives section.
                    ReturnError_(ParseInstancePrimitivesSection(context, root,
                                                                instancedCopyKV.value["instancedPrimitiveJsonFiles"], altScene));
                }

                if(sceneIndex != InvalidIndex64) {
                    copyInstance.index = sceneIndex;
                    rootScene->subsceneInstances.Add(copyInstance);
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
        AssetFileUtils::EnsureAssetDirectory<SubsceneResource>();
        AssetFileUtils::EnsureAssetDirectory<ModelResource>();
        return Success_;
    }

    //=============================================================================================================================
    cpointer CDisneySceneBuildProcessor::Type()
    {
        return "disneyscene";
    }

    //=============================================================================================================================
    uint64 CDisneySceneBuildProcessor::Version()
    {
        return SceneResource::kDataVersion + SubsceneResource::kDataVersion + ModelResource::kDataVersion;
    }

    //=============================================================================================================================
    Error CDisneySceneBuildProcessor::Process(BuildProcessorContext* context)
    {
        FixedString256 contentRoot = ContentRoot(context);

        SceneFileData sceneFile;
        ReturnError_(ParseSceneFile(context, sceneFile));

        CArray<SubsceneResourceData*> allScenes;

        SceneResourceData* rootScene = New_(SceneResourceData);
        rootScene->name.Copy(context->source.name.Ascii());
        rootScene->backgroundIntensity = float4::One_;
        rootScene->iblName.Copy(sceneFile.iblFile.Ascii());

        ReturnError_(ParseCameraFile(context, sceneFile.cameraFile.Ascii(), rootScene->camera));
        ReturnError_(ParseLightsFile(context, sceneFile.lightsFile.Ascii(), rootScene));
        
        for(uint scan = 0, count = sceneFile.elements.Count(); scan < count; ++scan) {
            const FilePathString& elementName = sceneFile.elements[scan];
            ReturnError_(ParseElementFile(context, contentRoot, elementName, rootScene, allScenes));
        }

        for(uint scan = 0, count = allScenes.Count(); scan < count; ++scan) {

            SubsceneResourceData* scene = allScenes[scan];
            for(uint scan = 0, count = scene->modelNames.Count(); scan < count; ++scan) {
                if(!StringUtil::EndsWithIgnoreCase(scene->modelNames[scan].Ascii(), CurveModelNameSuffix_)) {
                    context->AddProcessDependency("model", scene->modelNames[scan].Ascii());
                }
            }

            ReturnError_(context->CreateOutput(SubsceneResource::kDataType, SubsceneResource::kDataVersion,
                                               scene->name.Ascii(), *scene));
            Delete_(scene);
        }
        allScenes.Shutdown();

        if(StringUtil::Length(sceneFile.iblFile.Ascii()) > 0) {
            context->AddProcessDependency("DualIbl", sceneFile.iblFile.Ascii());
        }

        ReturnError_(context->CreateOutput(SceneResource::kDataType, SceneResource::kDataVersion,
                                           rootScene->name.Ascii(), *rootScene));
        Delete_(rootScene);

        return Success_;
    }
}