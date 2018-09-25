#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "TextureLib/TextureCache.h"
#include "GeometryLib/AxisAlignedBox.h"
#include "MathLib/FloatFuncs.h"
#include "SystemLib/BasicTypes.h"

struct RTCDeviceTy;
typedef struct RTCDeviceTy* RTCDevice;

struct RTCSceneTy;
typedef struct RTCSceneTy* RTCScene;

struct RTCGeometryTy;
typedef struct RTCGeometryTy* RTCGeometry;

namespace Selas
{
    struct MaterialResourceData;

    enum EmbreeGeometryFlags
    {
        HasNormals  = 1 << 0,
        HasTangents = 1 << 1,
        HasUvs      = 1 << 2
    };

    struct Material
    {
        const MaterialResourceData* resource;
        TextureHandle baseColorTextureHandle;
    };

    struct GeometryUserData
    {
        RTCScene rtcScene;
        RTCGeometry rtcGeometry;

        float4x4 worldToLocal;
        AxisAlignedBox aaBox;
        uint32 instanceID;

        Material material;
        uint32 flags;
    };
}
