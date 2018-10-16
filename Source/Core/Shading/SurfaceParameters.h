#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SceneLib/ModelResource.h"
#include "MathLib/FloatStructs.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    struct GIIntegratorContext;
    struct HitParameters;
    struct ModelResource;
    struct MaterialResourceData;

    struct SurfaceParameters
    {
        float3 position;
        float3x3 worldToTangent;
        float error;

        float3 view;
        
        float3 baseColor;
        float3 transmittanceColor;
        float sheen;
        float sheenTint;
        float clearcoat;
        float clearcoatGloss;
        float metallic;
        float specTrans;
        float diffTrans;
        float flatness;
        float anisotropic;
        float relativeIOR;
        float specularTint;
        float roughness;
        float scatterDistance;

        float ior;

        // -- material layer info
        ShaderType shader;
        uint32 materialFlags;

        uint32 lightSetIndex;
    };

    bool CalculateSurfaceParams(const GIIntegratorContext* context, const HitParameters* hit, SurfaceParameters& surface);
    bool CalculateSurfaceParams(const GIIntegratorContext* context, const HitParameters* hit,
                                ModelGeometryUserData* modelData, float4x4 localToWorld, Ptex::PtexFilter* filter,
                                SurfaceParameters& surface);

    bool CalculatePassesAlphaTest(const ModelGeometryUserData* geomData, uint32 geomId, uint32 primitiveId, float2 baryCoords);
    float CalculateDisplacement(const ModelGeometryUserData* geomData, RTCGeometry rtcGeometry, uint32 primId, float2 barys);

    float3 GeometricTangent(const SurfaceParameters& surface);
    float3 GeometricNormal(const SurfaceParameters& surface);
    float3 GeometricBitangent(const SurfaceParameters& surface);

    float3 OffsetRayOrigin(const SurfaceParameters& surface, float3 direction, float biasScale);
    float3 OffsetRayOrigin(const SurfaceParameters& surface, float3 direction, float biasScale, float& signedBiasDistance);

    float ContinuationProbability(const SurfaceParameters& surface);
}