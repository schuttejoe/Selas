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
    const uint64 SceneResource::kDataVersion = 1536806594ul;

    #define ModelInstanceMask_  0x0000FFFF
    #define SceneInstanceMask_  0xFFFF0000
    #define SceneInstanceShift_ 16

    //=============================================================================================================================
    static uint32 CalculateInstanceID(uint32 scene, uint32 model)
    {
        AssertMsg_((model & SceneInstanceMask_) == 0, "We don't support that many levels of instancing. Could be done by compiling"
                                                      "embree with RTC_MAX_INSTANCE_LEVEL_COUNT>1 though.");

        return (scene << SceneInstanceShift_) | (model & ModelInstanceMask_);
    }

    //=============================================================================================================================
    static void IDsFromInstanceID(uint32 instId, uint32& scene, uint32& model)
    {
        model = (instId & ModelInstanceMask_);
        scene = (instId >> SceneInstanceShift_);
    }

    struct SceneInstanceData
    {
        SceneResource* scene;
        float4x4 worldToLocal;
        AxisAlignedBox aaBox;
        uint32 sceneIdx;
    };

    //=============================================================================================================================
    // Serialization
    //=============================================================================================================================

    //=============================================================================================================================
    void Serialize(CSerializer* serializer, CurveSegment& data)
    {
        Serialize(serializer, data.startIndex);
        Serialize(serializer, data.controlPointCount);
    }

    //=============================================================================================================================
    void Serialize(CSerializer* serializer, CurveData& data)
    {
        Serialize(serializer, data.widthTip);
        Serialize(serializer, data.widthRoot);
        Serialize(serializer, data.degrees);
        Serialize(serializer, data.faceCamera);
        Serialize(serializer, data.controlPoints);
        Serialize(serializer, data.segments);
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
        Serialize(serializer, data.curves);
        Serialize(serializer, data.camera);
    }

    //=============================================================================================================================
    // SceneResource
    //=============================================================================================================================

    //=============================================================================================================================
    SceneResource::SceneResource()
        : data(nullptr)
        , sceneInstanceData(nullptr)
        , scenes(nullptr)
        , models(nullptr)
        , iblResource(nullptr)
    {

    }

    //=============================================================================================================================
    SceneResource::~SceneResource()
    {
        Assert_(data == nullptr);
        Assert_(sceneInstanceData == nullptr);
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
        SceneInstanceData* data = (SceneInstanceData*)args->geometryUserPtr;
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
        const SceneInstanceData* instance = (const SceneInstanceData*)args->geometryUserPtr;

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

        uint32 instId = CalculateInstanceID(instance->sceneIdx, 0);

        context->instID[0] = instId;
        rtcIntersect1(instance->scene->rtcScene, context, &rayhit);
        context->instID[0] = -1;
    }

    //=============================================================================================================================
    static void InstanceOccludedFunction(const RTCOccludedFunctionNArguments* args)
    {
        Assert_(args->N == 1);
        if(!args->valid[0])
            return;

        RTCIntersectContext* context = args->context;
        const SceneInstanceData* instance = (const SceneInstanceData*)args->geometryUserPtr;

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

        rtcOccluded1(instance->scene->rtcScene, context, &ray);
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
            scene->sceneInstanceData = AllocArray_(SceneInstanceData, scene->data->sceneInstances.Count());

            for(uint scan = 0, count = scene->data->sceneInstances.Count(); scan < count; ++scan) {

                const Instance& instance = scene->data->sceneInstances[scan];

                uint sceneIdx = instance.index;

                scene->sceneInstanceData[scan].worldToLocal = instance.worldToLocal;
                scene->sceneInstanceData[scan].scene = scene->scenes[sceneIdx];

                // -- Offset by one since "scene" is the 0th index. See ModelFromInstanceId for clarity.
                scene->sceneInstanceData[scan].sceneIdx = (uint32)scan + 1;

                MakeInvalid(&scene->sceneInstanceData[scan].aaBox);
                IncludeBox(&scene->sceneInstanceData[scan].aaBox, scene->data->sceneInstances[scan].localToWorld,
                           scene->scenes[sceneIdx]->aaBox);

                RTCGeometry geom = rtcNewGeometry(rtcDevice, RTC_GEOMETRY_TYPE_USER);

                rtcSetGeometryUserPrimitiveCount(geom, 1);
                rtcSetGeometryUserData(geom, &scene->sceneInstanceData[scan]);
                rtcSetGeometryBoundsFunction(geom, SceneInstanceBoundsFunction, nullptr);
                rtcSetGeometryIntersectFunction(geom, SceneInstanceIntersectFunction);
                rtcSetGeometryOccludedFunction(geom, InstanceOccludedFunction);
                rtcCommitGeometry(geom);
                rtcAttachGeometryByID(scene->rtcScene, geom, (uint32)(offset + scan));
                rtcReleaseGeometry(geom);
            }
        }
    }

    //=============================================================================================================================
    static void InitializeCurves(SceneResource* scene, RTCDevice rtcDevice)
    {
        //RTC_GEOMETRY_TYPE_ROUND_BSPLINE_CURVE
    }

    //=============================================================================================================================
    void InitializeEmbreeScene(SceneResource* scene, RTCDevice rtcDevice)
    {
        for(uint scan = 0, modelCount = scene->data->modelNames.Count(); scan < modelCount; ++scan) {
            InitializeEmbreeScene(scene->models[scan], rtcDevice);
        }

        for(uint scan = 0, sceneCount = scene->data->sceneNames.Count(); scan < sceneCount; ++scan) {
            InitializeEmbreeScene(scene->scenes[scan], rtcDevice);
        }

        scene->rtcScene = rtcNewScene(rtcDevice);

        InitializeModelInstances(scene, rtcDevice);
        InitializeSceneInstances(scene, rtcDevice);

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

        for(uint scan = 0, sceneCount = scene->data->sceneNames.Count(); scan < sceneCount; ++scan) {
            ShutdownSceneResource(scene->scenes[scan]);
            Delete_(scene->scenes[scan]);
        }

        SafeFree_(scene->sceneInstanceData);
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
            if(scene->models[scan]->data->cameraCount > 0) {
                InitializeRayCastCamera(scene->models[scan]->data->cameras[0], width, height, camera);
                return;
            }
        }

        // -- and finally fall back on the default
        CameraSettings defaultCamera;
        DefaultCameraSettings(&defaultCamera);
        InitializeRayCastCamera(defaultCamera, width, height, camera);
    }

    //=============================================================================================================================
    ModelResource* ModelFromInstanceId(const SceneResource* scene, int32 instId)
    {
        uint32 sceneId, modelId;
        IDsFromInstanceID(instId, sceneId, modelId);

        if(sceneId == 0) {
            uint modelIndex = scene->data->modelInstances[modelId].index;
            return scene->models[modelIndex];
        }

        uint sceneIndex = scene->data->sceneInstances[sceneId - 1].index;
        return ModelFromInstanceId(scene->scenes[sceneIndex], modelId);
    }
}
