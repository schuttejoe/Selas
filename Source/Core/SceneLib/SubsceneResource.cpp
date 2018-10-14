//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SceneLib/SubsceneResource.h"
#include "SceneLib/ModelResource.h"
#include "Assets/AssetFileUtils.h"
#include "MathLib/Trigonometric.h"
#include "MathLib/FloatFuncs.h"
#include "IoLib/BinaryStreamSerializer.h"
#include "SystemLib/BasicTypes.h"

#include "embree3/rtcore.h"
#include "embree3/rtcore_ray.h"

namespace Selas
{
    cpointer SubsceneResource::kDataType = "SubsceneResource";
    const uint64 SubsceneResource::kDataVersion = 1539129606ul;

    //=============================================================================================================================
    static uint64 EstimateSubsceneSize(SubsceneResource* subscene)
    {
        uint64 estimate = 0;
        for(uint scan = 0, count = subscene->data->modelNames.Count(); scan < count; ++scan) {
            estimate += subscene->models[scan]->geometrySize;
        }

        // Increasing our estimate to approximate the cost for embree's BVH data.
        return (uint)(estimate * 3.0f);
    }

    //=============================================================================================================================
    // Serialization
    //=============================================================================================================================

    //=============================================================================================================================
    void Serialize(CSerializer* serializer, SubsceneResourceData& data)
    {
        Serialize(serializer, data.name);
        Serialize(serializer, data.lightSetIndex);
        Serialize(serializer, data.modelNames);
        Serialize(serializer, data.modelInstances);
        Serialize(serializer, data.sceneMaterialNames);
        Serialize(serializer, data.sceneMaterials);
    }

    //=============================================================================================================================
    // SubsceneResource
    //=============================================================================================================================

    //=============================================================================================================================
    static void CalculateSubsceneBoundingBox(SubsceneResource* scene)
    {
        MakeInvalid(&scene->aaBox);
        for(uint scan = 0, count = scene->data->modelInstances.Count(); scan < count; ++scan) {
            uint modelIndex = scene->data->modelInstances[scan].index;
            IncludeBox(&scene->aaBox, scene->data->modelInstances[scan].localToWorld, scene->models[modelIndex]->data->aaBox);
        }
    }

    //=============================================================================================================================
    static void InitializeModelInstances(SubsceneResource* scene, RTCDevice rtcDevice)
    {
        for(uint scan = 0, count = scene->data->modelInstances.Count(); scan < count; ++scan) {
            RTCGeometry instance = rtcNewGeometry(rtcDevice, RTC_GEOMETRY_TYPE_INSTANCE);

            uint modelIdx = scene->data->modelInstances[scan].index;
            rtcSetGeometryInstancedScene(instance, scene->models[modelIdx]->rtcScene);
            rtcSetGeometryTimeStepCount(instance, 1);

            rtcSetGeometryTransform(instance, 0, RTC_FORMAT_FLOAT4X4_COLUMN_MAJOR,
                                    (void*)&scene->data->modelInstances[scan].localToWorld);

            rtcCommitGeometry(instance);
            rtcAttachGeometryByID(scene->rtcScene, instance, (int32)scan);
            rtcReleaseGeometry(instance);
        }
    }

    //=============================================================================================================================
    SubsceneResource::SubsceneResource()
        : data(nullptr)
        , rtcScene(nullptr)
        , models(nullptr)
        , refCount(0)
        , geometryLoaded(0)
        , geometryLoading()
        , lastAccessDt(0)
    {

    }

    //=============================================================================================================================
    SubsceneResource::~SubsceneResource()
    {
        Assert_(data == nullptr);
        Assert_(models == nullptr);
    }

    //=============================================================================================================================
    Error ReadSubsceneResource(cpointer assetname, SubsceneResource* data)
    {
        FilePathString filepath;
        AssetFileUtils::AssetFilePath(SubsceneResource::kDataType, SubsceneResource::kDataVersion, assetname, filepath);

        void* fileData = nullptr;
        uint64 fileSize = 0;
        ReturnError_(File::ReadWholeFile(filepath.Ascii(), &fileData, &fileSize));

        AttachToBinary(data->data, (uint8*)fileData, fileSize);

        return Success_;
    }

    //=============================================================================================================================
    Error InitializeSubsceneResource(SubsceneResource* subscene, RTCDevice rtcDevice, TextureCache* cache)
    {
        uint modelCount = subscene->data->modelNames.Count();
        if(modelCount > 0) {
            subscene->models = AllocArray_(ModelResource*, modelCount);
            for(uint scan = 0; scan < modelCount; ++scan) {
                subscene->models[scan] = New_(ModelResource);
                ReturnError_(ReadModelResource(subscene->data->modelNames[scan].Ascii(), subscene->models[scan]));

                InitializeModelResource(subscene->models[scan], subscene, subscene->data->modelNames[scan].Ascii(),
                                        subscene->data->lightSetIndex,
                                        subscene->data->sceneMaterialNames, subscene->data->sceneMaterials, cache);
            }
        }

        subscene->rtcDevice = rtcDevice;
        subscene->geometrySizeEstimate = EstimateSubsceneSize(subscene);

        CalculateSubsceneBoundingBox(subscene);

        return Success_;
    }

    //=============================================================================================================================
    void LoadSubsceneGeometry(SubsceneResource* subscene)
    {
        subscene->rtcScene = rtcNewScene(subscene->rtcDevice);

        for(uint scan = 0, modelCount = subscene->data->modelNames.Count(); scan < modelCount; ++scan) {
            LoadModelGeometry(subscene->models[scan], subscene->rtcDevice);
        }

        InitializeModelInstances(subscene, subscene->rtcDevice);

        rtcCommitScene(subscene->rtcScene);

        Assert_(subscene->geometryLoaded == 0);
        subscene->geometryLoaded = 1;
    }

    //=============================================================================================================================
    void UnloadSubsceneGeometry(SubsceneResource* subscene)
    {
        for(uint scan = 0, modelCount = subscene->data->modelNames.Count(); scan < modelCount; ++scan) {
            UnloadModelGeometry(subscene->models[scan]);
        }
        if(subscene->rtcScene != nullptr) {
            rtcReleaseScene(subscene->rtcScene);
            subscene->rtcScene = nullptr;
        }

        subscene->geometryLoaded = 0;
    }

    //=============================================================================================================================
    void ShutdownSubsceneResource(SubsceneResource* subscene, TextureCache* textureCache)
    {
        UnloadSubsceneGeometry(subscene);

        for(uint scan = 0, modelCount = subscene->data->modelNames.Count(); scan < modelCount; ++scan) {
            ShutdownModelResource(subscene->models[scan], textureCache);
            Delete_(subscene->models[scan]);
        }

        SafeFree_(subscene->models);
        SafeFreeAligned_(subscene->data);
    }

    //=============================================================================================================================
    void ModelDataFromRayIds(const SubsceneResource* scene, int32 modelID, int32 geomId,
                            float4x4& localToWorld, ModelGeometryUserData*& modelData)
    {
        uint modelCount = scene->data->modelInstances.Count();
        Assert_(modelID < modelCount);

        uint modelIndex = scene->data->modelInstances[modelID].index;
        modelData = &scene->models[modelIndex]->userDatas[geomId];
        localToWorld = scene->data->modelInstances[modelID].localToWorld;
    }
}
