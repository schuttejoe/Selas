#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SceneLib/SceneResource.h>
#include <GeometryLib/SurfaceDifferentials.h>
#include <MathLib/FloatStructs.h>
#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    struct KernelContext;
    struct HitParameters;
    struct SceneResource;
    struct Material;

    struct SurfaceParameters
    {
        float3 position;
        float3 perturbedNormal;
        float3 geometricNormal;
        float error;

        float3x3 worldToTangent;
        float3x3 tangentToWorld;

        // -- spatial differentials
        float3 dpdu, dpdv;

        // -- material layer info
        eMaterialShader shader;
        float3 emissive;
        uint32 materialFlags;
        float3 albedo;
        float  metalness;
        float3 specularColor;
        float  roughness;
        float  ior;

        // -- surface hit info
        float3 rxOrigin;
        float3 rxDirection;
        float3 ryOrigin;
        float3 ryDirection;

        // -- uv differentials.
        SurfaceDifferentials differentials;
    };

    bool CalculateSurfaceParams(const KernelContext* context, const HitParameters* hit, SurfaceParameters& surface);
    float3 OffsetRayOrigin(const SurfaceParameters& surface, float3 direction, float biasScale);
}