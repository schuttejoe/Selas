#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "SceneLib/SceneResource.h"
#include "MathLib/FloatStructs.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    struct GIIntegrationContext;
    struct HitParameters;
    struct SceneResource;
    struct Material;

    struct SurfaceParameters
    {
        float3 position;
        float3 perturbedNormal;
        float3 geometricNormal;
        float error;

        float3 view;
        float3x3 worldToTangent;

        // -- material layer info
        eMaterialShader shader;
        uint32 materialFlags;
        float3 albedo;
        float  metalness;
        float3 specularColor;
        float  roughness;
        float  subsurface;

        float currentIor;
        float exitIor; // -- only valid if total internal reflection doesn't occur
    };

    bool CalculateSurfaceParams(const GIIntegrationContext* context, const HitParameters* hit, SurfaceParameters& surface);
    bool CalculatePassesAlphaTest(const SceneResource* scene, uint32 geomId, uint32 primitiveId, float2 baryCoords);
    float CalculateDisplacement(const SceneResource* scene, uint32 geomId, uint32 primitiveId, float2 uvs);

    float3 OffsetRayOrigin(const SurfaceParameters& surface, float3 direction, float biasScale);
    float3 OffsetRayOrigin(const SurfaceParameters& surface, float3 direction, float biasScale, float& signedBiasDistance);

    float ContinuationProbability(const SurfaceParameters& surface);
}