//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SceneLib/SceneResource.h"
#include "SceneLib/ModelResource.h"
#include "SceneLib/ImageBasedLightResource.h"
#include "Assets/AssetFileUtils.h"
#include "MathLib/Trigonometric.h"
#include "IoLib/BinaryStreamSerializer.h"
#include "SystemLib/BasicTypes.h"

#include "embree3/rtcore.h"
#include "embree3/rtcore_ray.h"

namespace Selas
{
    cpointer SceneResource::kDataType = "SceneResource";
    const uint64 SceneResource::kDataVersion = 1535044939ul;

    //=============================================================================================================================
    // Serialization
    //=============================================================================================================================

    //=============================================================================================================================
    void Serialize(CSerializer* serializer, SceneResourceData& data)
    {
        Serialize(serializer, data.iblName);
        Serialize(serializer, data.backgroundIntensity);
        Serialize(serializer, data.modelNames);
        Serialize(serializer, data.modelInstances);
    }

    //=============================================================================================================================
    // SceneResource
    //=============================================================================================================================

    //=============================================================================================================================
    SceneResource::SceneResource()
        : data(nullptr)
        , models(nullptr)
        , iblResource(nullptr)
    {

    }

    //=============================================================================================================================
    SceneResource::~SceneResource()
    {
        Assert_(data == nullptr);
        Assert_(models == nullptr);
        Assert_(iblResource == nullptr);
    }

    //=============================================================================================================================
    Error ReadSceneResource(cpointer assetname, SceneResource* data)
    {
        FilePathString filepath;
        AssetFileUtils::AssetFilePath(SceneResource::kDataType, SceneResource::kDataVersion, assetname, filepath);

        void* fileData = nullptr;
        uint32 fileSize = 0;
        ReturnError_(File::ReadWholeFile(filepath.Ascii(), &fileData, &fileSize));

        AttachToBinary(data->data, (uint8*)fileData, fileSize);

        return Success_;
    }

    //=============================================================================================================================
    Error InitializeSceneResource(SceneResource* scene)
    {
        uint modelCount = scene->data->modelNames.Count();

        scene->models = AllocArray_(ModelResource*, modelCount);

        for(uint scan = 0; scan < modelCount; ++scan) {
            scene->models[scan] = New_(ModelResource);
            ReturnError_(ReadModelResource(scene->data->modelNames[scan].Ascii(), scene->models[scan]));

            InitializeModelResource(scene->models[scan]);
        }

        if(StringUtil::Length(scene->data->iblName.Ascii()) > 0) {
            scene->iblResource = New_(ImageBasedLightResource);
            ReturnError_(ReadImageBasedLightResource(scene->data->iblName.Ascii(), scene->iblResource));
        }

        // -- JSTODO - initialize the bounding sphere for VCM to work with an IBL.
        // -- ... though there's an argument to be made the using the scene's bounding sphere for ibl emission is absolutely
        // -- awful and I should be using something fit to the frustum.

        return Success_;
    }

    //=============================================================================================================================
    void InitializeEmbreeScene(SceneResource* scene, RTCDevice rtcDevice)
    {
        for(uint scan = 0, modelCount = scene->data->modelNames.Count(); scan < modelCount; ++scan) {
            InitializeEmbreeScene(scene->models[scan], rtcDevice);
        }

        scene->rtcScene = rtcNewScene(rtcDevice);

        for(uint scan = 0, count = scene->data->modelInstances.Count(); scan < count; ++scan) {
            RTCGeometry instance = rtcNewGeometry(rtcDevice, RTC_GEOMETRY_TYPE_INSTANCE);
            
            uint modelIdx = scene->data->modelInstances[scan].meshIndex;
            rtcSetGeometryInstancedScene(instance, scene->models[modelIdx]->rtcScene);
            rtcSetGeometryTimeStepCount(instance, 1);
            rtcSetGeometryTransform(instance, 0, RTC_FORMAT_FLOAT3X4_ROW_MAJOR,
                                    (void*)&scene->data->modelInstances[scan].transform);

            rtcCommitGeometry(instance);
            rtcAttachGeometryByID(scene->rtcScene, instance, (int32)scan);
            rtcReleaseGeometry(instance);
        }

        rtcCommitScene(scene->rtcScene);
    }

    //=============================================================================================================================
    void ShutdownSceneResource(SceneResource* scene)
    {
        if(scene->iblResource) {
            ShutdownImageBasedLightResource(scene->iblResource);
            SafeDelete_(scene->iblResource);
        }

        for(uint scan = 0, modelCount = scene->data->modelNames.Count(); scan < modelCount; ++scan) {
            ShutdownModelResource(scene->models[scan]);
            Delete_(scene->models[scan]);
        }

        SafeFree_(scene->models);
        SafeFreeAligned_(scene->data);
    }

    void InitializeSceneCamera(const SceneResource* scene, uint width, uint height, RayCastCameraSettings& camera)
    {
        // -- if the scene has a camera set on it use that.

        // -- otherwise search the models for a valid camera
        for(uint scan = 0, count = scene->data->modelNames.Count(); scan < count; ++scan) {
            if(scene->models[scan]->data->cameraCount > 0) {
                InitializeRayCastCamera(scene->models[scan]->data->cameras[0], width, height, camera);
                return;
            }
        }

        // -- and finally fall back on the default
        CameraSettings defaultCamera;
        defaultCamera.position = float3(0.0f, 0.0f, 5.0f);
        defaultCamera.lookAt = float3(0.0f, 0.0f, 0.0f);
        defaultCamera.up = float3(0.0f, 1.0f, 0.0f);
        defaultCamera.fov = 45.0f * Math::DegreesToRadians_;
        defaultCamera.znear = 0.1f;
        defaultCamera.zfar = 500.0f;

        InitializeRayCastCamera(defaultCamera, width, height, camera);
    }

    //=============================================================================================================================
    ModelResource* ModelFromInstanceId(const SceneResource* scene, int32 instId)
    {
        Assert_(instId < scene->data->modelInstances.Count());

        uint modelIndex = scene->data->modelInstances[instId].meshIndex;
        return scene->models[modelIndex];
    }
}
