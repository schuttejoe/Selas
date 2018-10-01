//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SceneLib/SceneResource.h"
#include "SceneLib/ModelResource.h"
#include "SceneLib/ImageBasedLightResource.h"
#include "Assets/AssetFileUtils.h"
#include "MathLib/Trigonometric.h"
#include "MathLib/FloatFuncs.h"
#include "IoLib/BinaryStreamSerializer.h"
#include "SystemLib/BasicTypes.h"

#include "embree3/rtcore.h"
#include "embree3/rtcore_ray.h"

namespace Selas
{
    cpointer SceneResource::kDataType = "SceneResource";
    const uint64 SceneResource::kDataVersion = 1538428687ul;

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
    void Serialize(CSerializer* serializer, SceneResourceData& data)
    {
        Serialize(serializer, data.name);
        Serialize(serializer, data.iblName);
        Serialize(serializer, data.backgroundIntensity);
        Serialize(serializer, data.sceneNames);
        Serialize(serializer, data.modelNames);
        Serialize(serializer, data.sceneInstances);
        Serialize(serializer, data.modelInstances);
        Serialize(serializer, data.lights);
        Serialize(serializer, data.sceneMaterialNames);
        Serialize(serializer, data.sceneMaterials);
        Serialize(serializer, data.camera);
    }

    //=============================================================================================================================
    // SceneResource
    //=============================================================================================================================

    //=============================================================================================================================
    SceneResource::SceneResource()
        : data(nullptr)
        , geomInstanceData(nullptr)
        , scenes(nullptr)
        , models(nullptr)
        , iblResource(nullptr)
    {

    }

    //=============================================================================================================================
    SceneResource::~SceneResource()
    {
        Assert_(data == nullptr);
        Assert_(geomInstanceData == nullptr);
        Assert_(scenes == nullptr);
        Assert_(models == nullptr);
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
    Error InitializeSceneResource(SceneResource* scene)
    {
        uint sceneCount = scene->data->sceneNames.Count();
        uint modelCount = scene->data->modelNames.Count();

        if(modelCount > 0) {
            scene->models = AllocArray_(ModelResource*, modelCount);
            for(uint scan = 0; scan < modelCount; ++scan) {
                scene->models[scan] = New_(ModelResource);
                ReturnError_(ReadModelResource(scene->data->modelNames[scan].Ascii(), scene->models[scan]));

                InitializeModelResource(scene->models[scan]);
            }
        }

        if(sceneCount > 0) {
            scene->scenes = AllocArray_(SceneResource*, sceneCount);

            for(uint scan = 0; scan < sceneCount; ++scan) {
                scene->scenes[scan] = New_(SceneResource);
                ReturnError_(ReadSceneResource(scene->data->sceneNames[scan].Ascii(), scene->scenes[scan]));

                InitializeSceneResource(scene->scenes[scan]);
            }
        }

        if(StringUtil::Length(scene->data->iblName.Ascii()) > 0) {
            scene->iblResource = New_(ImageBasedLightResource);
            ReturnError_(ReadImageBasedLightResource(scene->data->iblName.Ascii(), scene->iblResource));
        }

        MakeInvalid(&scene->aaBox);

        // -- Set up a bounding box for this scene that includes all child models and scenes.
        for(uint scan = 0, count = scene->data->sceneInstances.Count(); scan < count; ++scan) {
            uint sceneIndex = scene->data->sceneInstances[scan].index;
            IncludeBox(&scene->aaBox, scene->data->sceneInstances[scan].localToWorld, scene->scenes[sceneIndex]->aaBox);
        }
        for(uint scan = 0, count = scene->data->modelInstances.Count(); scan < count; ++scan) {
            uint modelIndex = scene->data->modelInstances[scan].index;
            IncludeBox(&scene->aaBox, scene->data->modelInstances[scan].localToWorld, scene->models[modelIndex]->data->aaBox);
        }

        // -- JSTODO - initialize the bounding sphere for VCM to work with an IBL.
        // -- ... though there's an argument to be made the using the scene's bounding sphere for ibl emission is absolutely
        // -- awful and I should be using something fit to the frustum.

        return Success_;
    }

    //=============================================================================================================================
    static void SceneInstanceBoundsFunction(const struct RTCBoundsFunctionArguments* args)
    {
        GeometryUserData* data = (GeometryUserData*)args->geometryUserPtr;
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
        Assert_(args->N == 1);
        if(!args->valid[0])
            return;

        RTCIntersectContext* context = args->context;
        const GeometryUserData* instance = (const GeometryUserData*)args->geometryUserPtr;

        RTCRayN* rays = RTCRayHitN_RayN(args->rayhit, args->N);
        RTCHitN* hits = RTCRayHitN_HitN(args->rayhit, args->N);

        float3 origin;
        float3 direction;

        const uint32 N = args->N;
        origin.x = RTCRayN_org_x(rays, N, 0);
        origin.y = RTCRayN_org_y(rays, N, 0);
        origin.z = RTCRayN_org_z(rays, N, 0);
        direction.x = RTCRayN_dir_x(rays, N, 0);
        direction.y = RTCRayN_dir_y(rays, N, 0);
        direction.z = RTCRayN_dir_z(rays, N, 0);

        float3 localOrigin = MatrixMultiplyPoint(origin, instance->worldToLocal);
        float3 localDirection = MatrixMultiplyVector(direction, instance->worldToLocal);

        RTCRayHit rayhit;
        rayhit.ray.org_x = localOrigin.x;
        rayhit.ray.org_y = localOrigin.y;
        rayhit.ray.org_z = localOrigin.z;
        rayhit.ray.dir_x = localDirection.x;
        rayhit.ray.dir_y = localDirection.y;
        rayhit.ray.dir_z = localDirection.z;
        rayhit.ray.tnear = RTCRayN_tnear(rays, N, 0);
        rayhit.ray.tfar  = RTCRayN_tfar(rays, N, 0);
        
        rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        rayhit.hit.primID = RTC_INVALID_GEOMETRY_ID;
        rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
        rayhit.hit.instID[1] = RTC_INVALID_GEOMETRY_ID;

        rtcIntersect1(instance->rtcScene, context, &rayhit);

        if(rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
            RTCRayN_tfar(rays, N, 0) = rayhit.ray.tfar;
            rtcCopyHitToHitN(hits, &rayhit.hit, N, 0);
            RTCHitN_instID(hits, N, 0, 0) = instance->instanceID;
            RTCHitN_instID(hits, N, 0, 1) = rayhit.hit.instID[0];
        }
    }

    //=============================================================================================================================
    static void InstanceOccludedFunction(const RTCOccludedFunctionNArguments* args)
    {
        Assert_(args->N == 1);
        if(!args->valid[0])
            return;

        RTCIntersectContext* context = args->context;
        const GeometryUserData* instance = (const GeometryUserData*)args->geometryUserPtr;

        RTCRayN* rays = args->ray;

        float3 origin;
        float3 direction;

        const uint32 N = args->N;
        origin.x = RTCRayN_org_x(rays, N, 0);
        origin.y = RTCRayN_org_y(rays, N, 0);
        origin.z = RTCRayN_org_z(rays, N, 0);
        direction.x = RTCRayN_dir_x(rays, N, 0);
        direction.y = RTCRayN_dir_y(rays, N, 0);
        direction.z = RTCRayN_dir_z(rays, N, 0);

        float3 localOrigin = MatrixMultiplyPoint(origin, instance->worldToLocal);
        float3 localDirection = MatrixMultiplyVector(direction, instance->worldToLocal);

        RTCRay ray;
        ray.org_x = localOrigin.x;
        ray.org_y = localOrigin.y;
        ray.org_z = localOrigin.z;
        ray.dir_x = localDirection.x;
        ray.dir_y = localDirection.y;
        ray.dir_z = localDirection.z;
        ray.tnear = RTCRayN_tnear(rays, N, 0);
        ray.tfar = RTCRayN_tfar(rays, N, 0);

        rtcOccluded1(instance->rtcScene, context, &ray);
        RTCRayN_tfar(rays, N, 0) = ray.tfar;
    }

    //=============================================================================================================================
    static void InitializeModelInstances(SceneResource* scene, RTCDevice rtcDevice)
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
    static void InitializeSceneInstances(SceneResource* scene, RTCDevice rtcDevice)
    {
        uint32 offset = (uint32)scene->data->modelInstances.Count();

        if(scene->data->sceneInstances.Count() > 0) {
            scene->geomInstanceData = AllocArray_(GeometryUserData, scene->data->sceneInstances.Count());

            for(uint scan = 0, count = scene->data->sceneInstances.Count(); scan < count; ++scan) {

                const Instance& instance = scene->data->sceneInstances[scan];
                uint sceneIdx = instance.index;

                RTCGeometry geom = rtcNewGeometry(rtcDevice, RTC_GEOMETRY_TYPE_USER);

                scene->geomInstanceData[scan].worldToLocal = instance.worldToLocal;
                scene->geomInstanceData[scan].rtcScene = scene->scenes[sceneIdx]->rtcScene;
                scene->geomInstanceData[scan].instanceID = offset + (uint32)scan;
                scene->geomInstanceData[scan].material.resource = nullptr;
                scene->geomInstanceData[scan].material.baseColorTextureHandle = TextureHandle();
                scene->geomInstanceData[scan].rtcGeometry = geom;
                scene->geomInstanceData[scan].flags = 0;

                MakeInvalid(&scene->geomInstanceData[scan].aaBox);
                IncludeBox(&scene->geomInstanceData[scan].aaBox, scene->data->sceneInstances[scan].localToWorld,
                           scene->scenes[sceneIdx]->aaBox);

                rtcSetGeometryUserPrimitiveCount(geom, 1);
                rtcSetGeometryUserData(geom, &scene->geomInstanceData[scan]);
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
    static void InitializeChildEmbreeScene(SceneResource* scene, SceneResourceData* root, TextureCache* cache, RTCDevice rtcDevice)
    {
        Assert_(scene->data->sceneInstances.Count() == 0);

        for(uint scan = 0, modelCount = scene->data->modelNames.Count(); scan < modelCount; ++scan) {
            InitializeEmbreeScene(scene->models[scan], root->sceneMaterialNames, root->sceneMaterials, cache, rtcDevice);
        }

        scene->rtcScene = rtcNewScene(rtcDevice);

        InitializeModelInstances(scene, rtcDevice);

        rtcCommitScene(scene->rtcScene);
    }

    //=============================================================================================================================
    void InitializeEmbreeScene(SceneResource* scene, TextureCache* cache, RTCDevice rtcDevice)
    {
        for(uint scan = 0, modelCount = scene->data->modelNames.Count(); scan < modelCount; ++scan) {
            InitializeEmbreeScene(scene->models[scan], scene->data->sceneMaterialNames, scene->data->sceneMaterials,
                                  cache, rtcDevice);
        }

        for(uint scan = 0, sceneCount = scene->data->sceneNames.Count(); scan < sceneCount; ++scan) {
            InitializeChildEmbreeScene(scene->scenes[scan], scene->data, cache, rtcDevice);
        }

        scene->rtcScene = rtcNewScene(rtcDevice);

        InitializeModelInstances(scene, rtcDevice);
        InitializeSceneInstances(scene, rtcDevice);

        rtcCommitScene(scene->rtcScene);
    }

    //=============================================================================================================================
    void ShutdownSceneResource(SceneResource* scene, TextureCache* textureCache)
    {
        if(scene->iblResource) {
            ShutdownImageBasedLightResource(scene->iblResource);
            SafeDelete_(scene->iblResource);
        }

        for(uint scan = 0, modelCount = scene->data->modelNames.Count(); scan < modelCount; ++scan) {
            ShutdownModelResource(scene->models[scan], textureCache);
            Delete_(scene->models[scan]);
        }

        for(uint scan = 0, sceneCount = scene->data->sceneNames.Count(); scan < sceneCount; ++scan) {
            ShutdownSceneResource(scene->scenes[scan], textureCache);
            Delete_(scene->scenes[scan]);
        }

        SafeFree_(scene->geomInstanceData);
        SafeFree_(scene->scenes);
        SafeFree_(scene->models);
        SafeFreeAligned_(scene->data);
    }

    //=============================================================================================================================
    void InitializeSceneCamera(const SceneResource* scene, uint width, uint height, RayCastCameraSettings& camera)
    {
        // -- if the scene has a camera set on it use that.
        if(ValidCamera(scene->data->camera)) {
            InitializeRayCastCamera(scene->data->camera, width, height, camera);
            return;
        }

        // -- otherwise search the models for a valid camera
        for(uint scan = 0, count = scene->data->modelNames.Count(); scan < count; ++scan) {
            if(scene->models[scan]->data->cameras.Count() > 0) {
                CameraSettings settings = scene->models[scan]->data->cameras[0];
                settings.fov = 50 * Math::DegreesToRadians_;
                InitializeRayCastCamera(settings, width, height, camera);

                return;
            }
        }

        // -- and finally fall back on the default
        CameraSettings defaultCamera;
        DefaultCameraSettings(&defaultCamera);
        InitializeRayCastCamera(defaultCamera, width, height, camera);
    }

    //=============================================================================================================================
    static void GeometryFromRayIds(const SceneResource* scene, int32 modelID, int32 geomId,
                                   float4x4& localToWorld, RTCGeometry& rtcGeometry)
    {
        uint modelCount = scene->data->modelInstances.Count();
        Assert_(modelID < modelCount);

        uint modelIndex = scene->data->modelInstances[modelID].index;
        rtcGeometry = scene->models[modelIndex]->rtcGeometries[geomId];
        localToWorld = scene->data->modelInstances[modelID].localToWorld;
    }

    //=============================================================================================================================
    void GeometryFromRayIds(const SceneResource* scene, const int32 instIds[MaxInstanceLevelCount_], int32 geomId,
                            float4x4& localToWorld, RTCGeometry& rtcGeometry)
    {
        static_assert(MaxInstanceLevelCount_ == RTC_MAX_INSTANCE_LEVEL_COUNT,
                      "Embree was compiled with different instance levels count");

        uint modelCount = scene->data->modelInstances.Count();
        uint sceneCount = scene->data->sceneInstances.Count();

        if(instIds[0] == RTC_INVALID_GEOMETRY_ID) {
            Assert_(false);
        }
        else {
            uint32 sceneID = instIds[0];
            uint32 subsceneID = instIds[1];

            if(sceneID < modelCount) {
                uint modelIndex = scene->data->modelInstances[sceneID].index;
                rtcGeometry = scene->models[modelIndex]->rtcGeometries[geomId];
                localToWorld = scene->data->modelInstances[sceneID].localToWorld;
                return;
            }

            uint sceneIndex = scene->data->sceneInstances[sceneID - modelCount].index;
            GeometryFromRayIds(scene->scenes[sceneIndex], subsceneID, geomId, localToWorld, rtcGeometry);
            localToWorld = MatrixMultiply(localToWorld, scene->data->sceneInstances[sceneID - modelCount].localToWorld);
        }
    }
}
