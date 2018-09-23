//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/CDisneyCurveBuildProcessor.h"

#include "BuildCommon/BakeModel.h"
#include "BuildCommon/ImportModel.h"
#include "BuildCommon/ImportMaterial.h"
#include "BuildCore/BuildContext.h"
#include "UtilityLib/JsonUtilities.h"
#include "SceneLib/SceneResource.h"
#include "Assets/AssetFileUtils.h"
#include "MathLib/FloatFuncs.h"
#include "SystemLib/Logging.h"
#include "SystemLib/MemoryAllocation.h"
#include "SystemLib/CheckedCast.h"

namespace Selas
{
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
    static void BuildCurveModel(CurveData* importedCurve, BuiltModel& curveModel)
    {
        uint controlPointCount = importedCurve->controlPoints.Count();
        uint curveCount = importedCurve->segments.Count();

        MakeInvalid(&curveModel.aaBox);

        uint curveIndex = 0;
        uint64 indexOffset = 0;

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
            curve.nameHash = importedCurve->name; // This isn't going to work for materials because the names are not a exact match

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
    Error CDisneyCurveBuildProcessor::Setup()
    {
        AssetFileUtils::EnsureAssetDirectory<ModelResource>();
        return Success_;
    }

    //=============================================================================================================================
    cpointer CDisneyCurveBuildProcessor::Type()
    {
        return "disneycurve";
    }

    //=============================================================================================================================
    uint64 CDisneyCurveBuildProcessor::Version()
    {
        return ModelResource::kDataVersion;
    }

    //=============================================================================================================================
    Error CDisneyCurveBuildProcessor::Process(BuildProcessorContext* context)
    {
        FilePathString documentFilePath;
        AssetFileUtils::AssetFilePath("disneycurve", ModelResource::kDataVersion, context->source.name.Ascii(),
                                      documentFilePath);

        ReturnError_(context->AddFileDependency(documentFilePath.Ascii()));

        rapidjson::Document document;
        ReturnError_(Json::OpenJsonDocument(documentFilePath.Ascii(), document));

        uint controlPointCount = 0;
        uint curveCount = 0;

        CurveData* curve = New_(CurveData);

        curve->name = context->id.name;

        Json::ReadFloat(document, "widthTip", curve->widthTip, 1.0f);
        Json::ReadFloat(document, "widthRoot", curve->widthRoot, 1.0f);
        Json::ReadFloat(document, "degrees", curve->degrees, 1.0f);
        Json::ReadBool(document, "faceCamera", curve->faceCamera, false);

        FilePathString controlPointsFile;
        Json::ReadFixedString(document, "controlPointsFile", controlPointsFile);
        ReturnError_(ParseControlPointsFile(context, controlPointsFile.Ascii(), curve));

        BuiltModel curveModel;
        BuildCurveModel(curve, curveModel);
        ReturnError_(BakeModel(context, curveModel));

        return Success_;
    }
}