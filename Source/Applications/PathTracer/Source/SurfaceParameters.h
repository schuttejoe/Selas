#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <GeometryLib/SurfaceDifferentials.h>
#include <MathLib/FloatStructs.h>
#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    struct SceneResource;
    struct Material;

    struct SurfaceParameters
    {
        float3 position;
        float3 normal;
        float error;

        float3x3 worldToTangent;
        float3x3 tangentToWorld;

        // -- spatial differentials
        float3 dpdu, dpdv;

        // -- material layer info
        float3 emissive;
        uint32 materialFlags;
        float3 albedo;
        float  metalness;
        float3 specularColor;
        float  roughness;

        // -- uv differentials.
        SurfaceDifferentials differentials;
    };

    void CalculateSurfaceParams(const SceneResource* scene, const Ray& ray, float3 position, float error, uint32 primitiveId, float2 barycentric, SurfaceParameters& surface);
    float3 OffsetRayOrigin(const SurfaceParameters& surface, float3 direction, float biasScale);
}