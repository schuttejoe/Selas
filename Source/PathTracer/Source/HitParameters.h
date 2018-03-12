#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>
#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    struct SceneResource;
    struct Material;

    struct HitParameters
    {
        float3 n;
        float3 dpdu, dpdv;
        float3 dndu, dndv;
        float2 uvs;

        Material* material;
    };

    void CalculateSurfaceParams(const SceneResource* scene, uint32 primitiveId, float2 barycentric, HitParameters& hitParameters);
}