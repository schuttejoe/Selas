#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "Shading/IntegratorContexts.h"
#include "SceneLib/EmbreeUtils.h"
#include "StringLib/FixedString.h"
#include "GeometryLib/AxisAlignedBox.h"
#include "GeometryLib/Camera.h"
#include "UtilityLib/MurmurHash.h"
#include "MathLib/FloatStructs.h"
#include "MathLib/FloatFuncs.h"
#include "ContainersLib/CArray.h"
#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"
#include "SystemLib/OSThreading.h"

namespace Selas
{
    class TextureCache;
    struct ModelResource;
    struct ImageBasedLightResource;
    
    struct Instance
    {
        Instance()
        {
            localToWorld = Matrix4x4::Identity();
            worldToLocal = Matrix4x4::Identity();
        }

        uint64 index;
        float4x4 localToWorld;
        float4x4 worldToLocal;
    };

    //=============================================================================================================================
    struct SubsceneResourceData
    {
        FilePathString name;
        uint64 lightSetIndex;
        CArray<FilePathString> modelNames;
        CArray<Instance> modelInstances;
        CArray<Hash32> sceneMaterialNames;
        CArray<MaterialResourceData> sceneMaterials;
    };

    //=============================================================================================================================
    struct SubsceneResource
    {
        static cpointer kDataType;
        static const uint64 kDataVersion;

        SubsceneResourceData* data;

        RTCDevice rtcDevice;
        RTCScene rtcScene;

        AxisAlignedBox aaBox;
        float4 boundingSphere;
        uint64 geometrySizeEstimate;

        ModelResource** models;

        Align_(CacheLineSize_) volatile int64 refCount;
        Align_(CacheLineSize_) volatile int64 geometryLoaded;
        Align_(CacheLineSize_) volatile int64 geometryLoading;
        Align_(CacheLineSize_) volatile int64 lastAccessDt;

        SubsceneResource();
        ~SubsceneResource();
    };

    void Serialize(CSerializer* serializer, SubsceneResourceData& data);

    Error ReadSubsceneResource(cpointer filepath, SubsceneResource* scene);

    Error InitializeSubsceneResource(SubsceneResource* subscene, RTCDevice rtcDevice, TextureCache* textureCache);
    void LoadSubsceneGeometry(SubsceneResource* subscene);
    void UnloadSubsceneGeometry(SubsceneResource* subscene);
    void ShutdownSubsceneResource(SubsceneResource* scene, TextureCache* textureCache);

    void ModelDataFromRayIds(const SubsceneResource* scene, int32 modelID, int32 geomId,
                            float4x4& localToWorld, ModelGeometryUserData*& modelData);
}
