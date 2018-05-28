#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SceneLib/SceneResource.h>
#include <GeometryLib/SurfaceDifferentials.h>
#include <MathLib/FloatStructs.h>
#include <SystemLib/BasicTypes.h>

#define EnableDifferentials_ 0

namespace Selas
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

        float3 view;

        float3x3 worldToTangent;

        // -- material layer info
        eMaterialShader shader;
        uint32 materialFlags;
        float3 albedo;
        float  metalness;
        float3 specularColor;
        float  roughness;

        float currentIor;
        float exitIor; // -- only valid if total internal reflection doesn't occur

        #if EnableDifferentials_
            // -- spatial differentials
            float3 dpdu, dpdv;
            float3 rxDirection;
            float3 ryDirection;

            // -- uv differentials.
            SurfaceDifferentials differentials;
        #endif
    };

    bool CalculateSurfaceParams(const KernelContext* context, const Ray& ray, const HitParameters* hit, SurfaceParameters& surface);
    float3 OffsetRayOrigin(const SurfaceParameters& surface, float3 direction, float biasScale);
    float3 OffsetRayOrigin(const SurfaceParameters& surface, float3 direction, float biasScale, float& signedBiasDistance);
}