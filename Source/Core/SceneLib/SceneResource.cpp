//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SceneLib/SceneResource.h"
#include "SceneLib/ModelResource.h"
#include "SceneLib/ImageBasedLightResource.h"
#include "SceneLib/GeometryCache.h"
#include "Assets/AssetFileUtils.h"
#include "MathLib/Trigonometric.h"
#include "MathLib/FloatFuncs.h"
#include "IoLib/BinaryStreamSerializer.h"
#include "SystemLib/BasicTypes.h"
#include "SystemLib/Atomic.h"
#include "SystemLib/SystemTime.h"

#include "embree3/rtcore.h"
#include "embree3/rtcore_ray.h"

namespace Selas
{
    cpointer SceneResource::kDataType = "SceneResource";
    const uint64 SceneResource::kDataVersion = 1539147471ul;

    struct SubsceneInstanceUserData
    {
        GeometryCache* geometryCache;
        SubsceneResource* subscene;
        float4x4 worldToLocal;
        AxisAlignedBox aaBox;
        uint32 instanceID;
    };

    //=============================================================================================================================
    // Serialization
    //=============================================================================================================================

    static void Serialize(CSerializer* serializer, SceneLight& light)
    {
        Serialize(serializer, light.type);
        Serialize(serializer, light.position);
        Serialize(serializer, light.direction);
        Serialize(serializer, light.x);
        Serialize(serializer, light.z);
        Serialize(serializer, light.radiance);
    }

    //=============================================================================================================================
    static void Serialzie(CSerializer* serializer, SceneLightSetRange& lightSetRange)
    {
        Serialize(serializer, lightSetRange.start);
        Serialize(serializer, lightSetRange.count);
    }

    //=============================================================================================================================
    void Serialize(CSerializer* serializer, SceneResourceData& data)
    {
        Serialize(serializer, data.name);
        Serialize(serializer, data.iblName);
        Serialize(serializer, data.backgroundIntensity);
        Serialize(serializer, data.subsceneNames);
        Serialize(serializer, data.subsceneInstances);
        Serialize(serializer, data.lights);
        Serialize(serializer, data.lightSetRanges);
        Serialize(serializer, data.cameras);
    }

    //=============================================================================================================================
    // Embree callback functions
    //=============================================================================================================================

    //=============================================================================================================================
    static void SceneInstanceBoundsFunction(const struct RTCBoundsFunctionArguments* args)
    {
        SubsceneInstanceUserData* data = (SubsceneInstanceUserData*)args->geometryUserPtr;
        RTCBounds* bounds = args->bounds_o;

        bounds->lower_x = data->aaBox.min.x;
        bounds->lower_y = data->aaBox.min.y;
        bounds->lower_z = data->aaBox.min.z;
        bounds->upper_x = data->aaBox.max.x;
        bounds->upper_y = data->aaBox.max.y;
        bounds->upper_z = data->aaBox.max.z;
    }

    //=============================================================================================================================
    static void SceneInstanceIntersectFunction(const RTCIntersectFunctionNArguments* args)
    {
        RTCIntersectContext* context = args->context;
        const SubsceneInstanceUserData* instance = (const SubsceneInstanceUserData*)args->geometryUserPtr;

        RTCRayN* rays = RTCRayHitN_RayN(args->rayhit, args->N);
        RTCHitN* hits = RTCRayHitN_HitN(args->rayhit, args->N);

        const uint32 N = args->N;

        instance->geometryCache->EnsureSubsceneGeometryLoaded(instance->subscene);

        for(uint32 scan = 0; scan < N; ++scan) {
            if(args->valid[scan] == 0)
                continue;

            float3 origin;
            float3 direction;
            
            origin.x = RTCRayN_org_x(rays, N, scan);
            origin.y = RTCRayN_org_y(rays, N, scan);
            origin.z = RTCRayN_org_z(rays, N, scan);
            direction.x = RTCRayN_dir_x(rays, N, scan);
            direction.y = RTCRayN_dir_y(rays, N, scan);
            direction.z = RTCRayN_dir_z(rays, N, scan);

            float3 localOrigin = MatrixMultiplyPoint(origin, instance->worldToLocal);
            float3 localDirection = MatrixMultiplyVector(direction, instance->worldToLocal);

            RTCRayHit rayhit;
            rayhit.ray.org_x = localOrigin.x;
            rayhit.ray.org_y = localOrigin.y;
            rayhit.ray.org_z = localOrigin.z;
            rayhit.ray.dir_x = localDirection.x;
            rayhit.ray.dir_y = localDirection.y;
            rayhit.ray.dir_z = localDirection.z;
            rayhit.ray.tnear = RTCRayN_tnear(rays, N, scan);
            rayhit.ray.tfar = RTCRayN_tfar(rays, N, scan);

            rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
            rayhit.hit.primID = RTC_INVALID_GEOMETRY_ID;
            rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
            rayhit.hit.instID[1] = RTC_INVALID_GEOMETRY_ID;

            rtcIntersect1(instance->subscene->rtcScene, context, &rayhit);

            if(rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
                RTCRayN_tfar(rays, N, scan) = rayhit.ray.tfar;
                rtcCopyHitToHitN(hits, &rayhit.hit, N, scan);
                RTCHitN_instID(hits, N, scan, 0) = instance->instanceID;
                RTCHitN_instID(hits, N, scan, 1) = rayhit.hit.instID[0];
            }
        }

        instance->geometryCache->FinishUsingSubceneGeometry(instance->subscene);
    }

    //=============================================================================================================================
    static void InstanceOccludedFunction(const RTCOccludedFunctionNArguments* args)
    {
        RTCIntersectContext* context = args->context;
        const SubsceneInstanceUserData* instance = (const SubsceneInstanceUserData*)args->geometryUserPtr;

        RTCRayN* rays = args->ray;

        const uint32 N = args->N;

        instance->geometryCache->EnsureSubsceneGeometryLoaded(instance->subscene);
        for(uint32 scan = 0; scan < N; ++scan) {
            if(args->valid[scan] == 0)
                continue;

            float3 origin;
            float3 direction;

            origin.x = RTCRayN_org_x(rays, N, scan);
            origin.y = RTCRayN_org_y(rays, N, scan);
            origin.z = RTCRayN_org_z(rays, N, scan);
            direction.x = RTCRayN_dir_x(rays, N, scan);
            direction.y = RTCRayN_dir_y(rays, N, scan);
            direction.z = RTCRayN_dir_z(rays, N, scan);

            float3 localOrigin = MatrixMultiplyPoint(origin, instance->worldToLocal);
            float3 localDirection = MatrixMultiplyVector(direction, instance->worldToLocal);

            RTCRay ray;
            ray.org_x = localOrigin.x;
            ray.org_y = localOrigin.y;
            ray.org_z = localOrigin.z;
            ray.dir_x = localDirection.x;
            ray.dir_y = localDirection.y;
            ray.dir_z = localDirection.z;
            ray.tnear = RTCRayN_tnear(rays, N, scan);
            ray.tfar = RTCRayN_tfar(rays, N, scan);

            rtcOccluded1(instance->subscene->rtcScene, context, &ray);

            RTCRayN_tfar(rays, N, scan) = ray.tfar;
        }
        instance->geometryCache->FinishUsingSubceneGeometry(instance->subscene);
    }

    //=============================================================================================================================
    // SceneResource
    //=============================================================================================================================

    //=============================================================================================================================
    static void CalculateSceneBoundingBox(SceneResource* scene)
    {
        MakeInvalid(&scene->aaBox);

        // -- Set up a bounding box for this scene that includes all child models and scenes.
        for(uint scan = 0, count = scene->data->subsceneInstances.Count(); scan < count; ++scan) {
            uint sceneIndex = scene->data->subsceneInstances[scan].index;
            IncludeBox(&scene->aaBox, scene->data->subsceneInstances[scan].localToWorld, scene->subscenes[sceneIndex]->aaBox);
        }
    }

    //=============================================================================================================================
    static void CreateSceneLightSets(SceneResource* scene)
    {
        scene->lightSets.Resize(scene->data->lightSetRanges.Count());
        for(uint scan = 0, count = scene->data->lightSetRanges.Count(); scan < count; ++scan) {
            const SceneLightSetRange& range = scene->data->lightSetRanges[scan];

            scene->lightSets[scan].lights = scene->data->lights.DataPointer() + range.start;
            scene->lightSets[scan].count = range.count;
        }
    }

    //=============================================================================================================================
    static void SetupSceneInstances(SceneResource* scene, RTCDevice rtcDevice, GeometryCache* geometryCache)
    {
        if(scene->data->subsceneInstances.Count() > 0) {
            scene->subsceneInstanceUserDatas = AllocArray_(SubsceneInstanceUserData, scene->data->subsceneInstances.Count());

            for(uint scan = 0, count = scene->data->subsceneInstances.Count(); scan < count; ++scan) {

                const Instance& instance = scene->data->subsceneInstances[scan];
                uint sceneIdx = instance.index;

                RTCGeometry geom = rtcNewGeometry(rtcDevice, RTC_GEOMETRY_TYPE_USER);

                scene->subsceneInstanceUserDatas[scan].geometryCache = geometryCache;
                scene->subsceneInstanceUserDatas[scan].worldToLocal = MatrixInverse(instance.localToWorld);
                scene->subsceneInstanceUserDatas[scan].subscene = scene->subscenes[sceneIdx];
                scene->subsceneInstanceUserDatas[scan].instanceID = (uint32)scan;

                MakeInvalid(&scene->subsceneInstanceUserDatas[scan].aaBox);
                IncludeBox(&scene->subsceneInstanceUserDatas[scan].aaBox, scene->data->subsceneInstances[scan].localToWorld,
                           scene->subscenes[sceneIdx]->aaBox);

                rtcSetGeometryUserPrimitiveCount(geom, 1);
                rtcSetGeometryUserData(geom, &scene->subsceneInstanceUserDatas[scan]);
                rtcSetGeometryBoundsFunction(geom, SceneInstanceBoundsFunction, nullptr);
                rtcSetGeometryIntersectFunction(geom, SceneInstanceIntersectFunction);
                rtcSetGeometryOccludedFunction(geom, InstanceOccludedFunction);
                rtcCommitGeometry(geom);
                rtcAttachGeometry(scene->rtcScene, geom);
                rtcReleaseGeometry(geom);
            }
        }
    }

    //=============================================================================================================================
    SceneResource::SceneResource()
        : data(nullptr)
        , subsceneInstanceUserDatas(nullptr)
        , subscenes(nullptr)
        , iblResource(nullptr)
    {

    }

    //=============================================================================================================================
    SceneResource::~SceneResource()
    {
        Assert_(data == nullptr);
        Assert_(subsceneInstanceUserDatas == nullptr);
        Assert_(subscenes == nullptr);
        Assert_(iblResource == nullptr);
    }

    //=============================================================================================================================
    Error ReadSceneResource(cpointer assetname, SceneResource* data)
    {
        FilePathString filepath;
        AssetFileUtils::AssetFilePath(SceneResource::kDataType, SceneResource::kDataVersion, assetname, filepath);

        void* fileData = nullptr;
        uint64 fileSize = 0;
        ReturnError_(File::ReadWholeFile(filepath.Ascii(), &fileData, &fileSize));

        AttachToBinary(data->data, (uint8*)fileData, fileSize);

        return Success_;
    }

    //=============================================================================================================================
    Error InitializeSceneResource(SceneResource* scene, TextureCache* textureCache, GeometryCache* geometryCache,
                                  RTCDevice rtcDevice)
    {
        uint subsceneCount = scene->data->subsceneNames.Count();

        if(subsceneCount > 0) {
            scene->subscenes = AllocArray_(SubsceneResource*, subsceneCount);

            for(uint scan = 0; scan < subsceneCount; ++scan) {
                scene->subscenes[scan] = New_(SubsceneResource);
                ReturnError_(ReadSubsceneResource(scene->data->subsceneNames[scan].Ascii(), scene->subscenes[scan]));

                ReturnError_(InitializeSubsceneResource(scene->subscenes[scan], rtcDevice, textureCache));
            }
        }

        if(StringUtil::Length(scene->data->iblName.Ascii()) > 0) {
            scene->iblResource = New_(ImageBasedLightResource);
            ReturnError_(ReadImageBasedLightResource(scene->data->iblName.Ascii(), scene->iblResource));
        }

        CalculateSceneBoundingBox(scene);
        CreateSceneLightSets(scene);

        scene->rtcScene = rtcNewScene(rtcDevice);
        SetupSceneInstances(scene, rtcDevice, geometryCache);
        rtcCommitScene(scene->rtcScene);

        return Success_;
    }

    //=============================================================================================================================
    void ShutdownSceneResource(SceneResource* scene, TextureCache* textureCache)
    {
        if(scene->iblResource) {
            ShutdownImageBasedLightResource(scene->iblResource);
            SafeDelete_(scene->iblResource);
        }

        for(uint scan = 0, sceneCount = scene->data->subsceneNames.Count(); scan < sceneCount; ++scan) {
            ShutdownSubsceneResource(scene->subscenes[scan], textureCache);
            Delete_(scene->subscenes[scan]);
        }
      
        SafeFree_(scene->subsceneInstanceUserDatas);
        SafeFree_(scene->subscenes);
        SafeFreeAligned_(scene->data);
    }

    //=============================================================================================================================
    void SetupSceneCamera(const SceneResource* scene, uint index, uint width, uint height, RayCastCameraSettings& camera)
    {
        // -- if the scene has a camera set on it use that.
        if(ValidCamera(scene->data->cameras[index])) {
            InitializeRayCastCamera(scene->data->cameras[index], width, height, camera);
            return;
        }

        //// -- otherwise search the models for a valid camera
        //for(uint scan = 0, count = scene->data->modelNames.Count(); scan < count; ++scan) {
        //    if(scene->models[scan]->data->cameras.Count() > 0) {
        //        CameraSettings settings = scene->models[scan]->data->cameras[0];
        //        settings.fov = 50 * Math::DegreesToRadians_;
        //        InitializeRayCastCamera(settings, width, height, camera);

        //        return;
        //    }
        //}

        // -- and finally fall back on the default
        CameraSettings defaultCamera;
        DefaultCameraSettings(&defaultCamera);
        InitializeRayCastCamera(defaultCamera, width, height, camera);
    }

    //=============================================================================================================================
    void ModelDataFromRayIds(const SceneResource* scene, const int32 instIds[MaxInstanceLevelCount_], int32 geomId,
                            float4x4& localToWorld, ModelGeometryUserData*& modelData)
    {
        static_assert(MaxInstanceLevelCount_ == RTC_MAX_INSTANCE_LEVEL_COUNT,
                      "Embree was compiled with different instance levels count");

        uint sceneCount = scene->data->subsceneInstances.Count();

        Assert_(instIds[0] != RTC_INVALID_GEOMETRY_ID);

        uint32 sceneID = instIds[0];
        uint32 subsceneID = instIds[1];

        uint sceneIndex = scene->data->subsceneInstances[sceneID].index;
        ModelDataFromRayIds(scene->subscenes[sceneIndex], subsceneID, geomId, localToWorld, modelData);
        localToWorld = MatrixMultiply(localToWorld, scene->data->subsceneInstances[sceneID].localToWorld);
    }
}
