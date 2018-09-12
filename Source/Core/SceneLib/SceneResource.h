#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "StringLib/FixedString.h"
#include "GeometryLib/AxisAlignedBox.h"
#include "GeometryLib/Camera.h"
#include "UtilityLib/MurmurHash.h"
#include "MathLib/FloatStructs.h"
#include "ContainersLib/CArray.h"
#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"

// JSTODO - Make a common header for these?
struct RTCDeviceTy;
typedef struct RTCDeviceTy* RTCDevice;

struct RTCSceneTy;
typedef struct RTCSceneTy* RTCScene;

namespace Selas
{
    struct ModelResource;
    struct ImageBasedLightResource;
    struct SceneInstanceData;

    struct Instance
    {
        uint64 index;
        float4x4 localToWorld;
        float4x4 worldToLocal;
    };

    struct SceneResourceData
    {
        FilePathString name;
        FilePathString iblName;
        float4 backgroundIntensity;
        CArray<FilePathString> sceneNames;
        CArray<FilePathString> modelNames;
        CArray<Instance> sceneInstances;
        CArray<Instance> modelInstances;
        CameraSettings camera;
    };

    struct SceneResource
    {
        static cpointer kDataType;
        static const uint64 kDataVersion;

        SceneResourceData* data;

        RTCScene rtcScene;

        AxisAlignedBox aaBox;
        float4 boundingSphere;

        SceneInstanceData* sceneInstanceData;
        SceneResource** scenes;
        ModelResource** models;
        ImageBasedLightResource* iblResource;

        SceneResource();
        ~SceneResource();
    };

    void Serialize(CSerializer* serializer, SceneResourceData& data);

    Error ReadSceneResource(cpointer filepath, SceneResource* scene);
    Error InitializeSceneResource(SceneResource* scene);
    void InitializeEmbreeScene(SceneResource* scene, RTCDevice rtcDevice);
    void ShutdownSceneResource(SceneResource* scene);

    void InitializeSceneCamera(const SceneResource* scene, uint width, uint height, RayCastCameraSettings& camera);
    ModelResource* ModelFromInstanceId(const SceneResource* scene, int32 instId);
}
