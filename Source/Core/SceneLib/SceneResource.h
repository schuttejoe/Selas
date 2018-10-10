#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SceneLib/EmbreeUtils.h"
#include "SceneLib/SubsceneResource.h"
#include "Shading/IntegratorContexts.h"
#include "StringLib/FixedString.h"
#include "GeometryLib/AxisAlignedBox.h"
#include "GeometryLib/Camera.h"
#include "UtilityLib/MurmurHash.h"
#include "MathLib/FloatStructs.h"
#include "ContainersLib/CArray.h"
#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    class GeometryCache;
    class TextureCache;
    struct SubsceneResource;
    struct ImageBasedLightResource;
    struct SubsceneInstanceUserData;

    enum SubsceneLightType
    {
        QuadLight
    };

    struct SceneLight
    {
        uint32 type;
        float3 position;
        float3 direction;
        float3 x;
        float3 z;
        float3 radiance;
    };

    struct SceneLightSet
    {
        CArray<SceneLight> lights;
    };

    //=============================================================================================================================
    struct SceneResourceData
    {
        FilePathString name;
        FilePathString iblName;
        float4 backgroundIntensity;
        CArray<FilePathString> subsceneNames;
        CArray<Instance> subsceneInstances;
        CArray<SceneLightSet> lightsets;
        
        CameraSettings camera;
    };

    //=============================================================================================================================
    struct SceneResource
    {
        static cpointer kDataType;
        static const uint64 kDataVersion;

        SceneResourceData* data;

        RTCScene rtcScene;

        AxisAlignedBox aaBox;
        float4 boundingSphere;

        SubsceneInstanceUserData* subsceneInstanceUserDatas;
        SubsceneResource** subscenes;
        ImageBasedLightResource* iblResource;

        SceneResource();
        ~SceneResource();
    };

    void Serialize(CSerializer* serializer, SceneResourceData& data);

    Error ReadSceneResource(cpointer filepath, SceneResource* scene);
    Error InitializeSceneResource(SceneResource* scene, TextureCache* cache, GeometryCache* geometryCache, RTCDevice rtcDevice);
    void ShutdownSceneResource(SceneResource* scene, TextureCache* textureCache);

    void SetupSceneCamera
    (const SceneResource* scene, uint width, uint height, RayCastCameraSettings& camera);

    void ModelDataFromRayIds(const SceneResource* scene, const int32 instIds[MaxInstanceLevelCount_], int32 geomId,
                            float4x4& localToWorld, ModelGeometryUserData*& modelData);
}
