//==============================================================================
// Joe Schutte
//==============================================================================

#include "HitParameters.h"
#include <SceneLib/SceneResource.h>
#include <MathLib/FloatFuncs.h>

namespace Shooty
{
    //==============================================================================
    static void CoordinateSystem(const float3& v1, float3* v2, float3* v3)
    {
        if(Math::Absf(v1.x) > Math::Absf(v1.y))
            *v2 = float3(-v1.z, 0, v1.x) * (1.0f / Math::Sqrtf(v1.x * v1.x + v1.z * v1.z));
        else
            *v2 = float3(0, v1.z, -v1.y) * (1.0f / Math::Sqrtf(v1.y * v1.y + v1.z * v1.z));
        *v3 = Cross(v1, *v2);
    }

    //==============================================================================
    void CalculateSurfaceParams(const SceneResource* __restrict scene, uint32 primitiveId, float2 barycentric, HitParameters& hitParameters)
    {
        uint32 i0 = scene->data->indices[3 * primitiveId + 0];
        uint32 i1 = scene->data->indices[3 * primitiveId + 1];
        uint32 i2 = scene->data->indices[3 * primitiveId + 2];

        const VertexAuxiliaryData& v0 = scene->data->vertexData[i0];
        const VertexAuxiliaryData& v1 = scene->data->vertexData[i1];
        const VertexAuxiliaryData& v2 = scene->data->vertexData[i2];

        float3 p0 = float3(v0.px, v0.py, v0.pz);
        float3 p1 = float3(v1.px, v1.py, v1.pz);
        float3 p2 = float3(v2.px, v2.py, v2.pz);
        float3 n0 = float3(v0.nx, v0.ny, v0.nz);
        float3 n1 = float3(v1.nx, v1.ny, v1.nz);
        float3 n2 = float3(v2.nx, v2.ny, v2.nz);
        float2 uv0 = float2(v0.u, v0.v);
        float2 uv1 = float2(v1.u, v1.v);
        float2 uv2 = float2(v2.u, v2.v);

        float b0 = Saturate(1.0f - (barycentric.x + barycentric.y));
        float b1 = barycentric.x;
        float b2 = barycentric.y;

        // Compute deltas for triangle partial derivatives
        float2 duv02 = uv0 - uv2;
        float2 duv12 = uv1 - uv2;
        float determinant = duv02.x * duv12.y - duv02.y * duv12.x;
        bool degenerateUV = Math::Absf(determinant) < SmallFloatEpsilon_;
        if(!degenerateUV) {
            float3 edge02 = p0 - p2;
            float3 edge12 = p1 - p2;
            float3 dn02 = n0 - n2;
            float3 dn12 = n1 - n2;

            float invDet = 1 / determinant;
            hitParameters.dpdu = (duv12.y * edge02 - duv02.y * edge12) * invDet;
            hitParameters.dpdv = (-duv12.x * edge02 + duv02.x * edge12) * invDet;
            hitParameters.dndu = (duv12.y * dn02 - duv02.y * dn12) * invDet;
            hitParameters.dndv = (-duv12.x * dn02 + duv02.x * dn12) * invDet;
        }
        if(degenerateUV || LengthSquared(Cross(hitParameters.dpdu, hitParameters.dpdv)) == 0.0f) {
            CoordinateSystem(Normalize(Cross(p2 - p0, p1 - p0)), &hitParameters.dpdu, &hitParameters.dpdv);
            hitParameters.dndu = float3::Zero_;
            hitParameters.dndv = float3::Zero_;
        }

        hitParameters.n = Normalize(b0 * n0 + b1 * n1 + b2 * n2);
        hitParameters.uvs = b0 * uv0 + b1 * uv1 + b2 * uv2;
        hitParameters.material = &scene->data->materials[v0.materialIndex];
    }
}