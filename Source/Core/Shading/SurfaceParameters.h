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
    struct Material;

    struct SurfaceParameters
    {
        float3 position;
        float3 perturbedNormal;
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
        eMaterialShader shader;
        uint32 materialFlags;
    };

    bool CalculateSurfaceParams(const GIIntegratorContext* context, const HitParameters* hit, SurfaceParameters& surface);
    bool CalculatePassesAlphaTest(const ModelResource* scene, uint32 geomId, uint32 primitiveId, float2 baryCoords);
    float CalculateDisplacement(const ModelResource* scene, uint32 geomId, uint32 primitiveId, float2 uvs);

    float3 GeometricTangent(const SurfaceParameters& surface);
    float3 GeometricNormal(const SurfaceParameters& surface);
    float3 GeometricBitangent(const SurfaceParameters& surface);

    float3 OffsetRayOrigin(const SurfaceParameters& surface, float3 direction, float biasScale);
    float3 OffsetRayOrigin(const SurfaceParameters& surface, float3 direction, float biasScale, float& signedBiasDistance);

    float ContinuationProbability(const SurfaceParameters& surface);
}