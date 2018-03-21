//==============================================================================
// Joe Schutte
//==============================================================================

#include "SurfaceParameters.h"
#include <TextureLib/TextureFiltering.h>
#include <TextureLib/TextureResource.h>
#include <SceneLib/SceneResource.h>
#include <GeometryLib/Ray.h>
#include <MathLib/FloatFuncs.h>
#include <MathLib/ColorSpace.h>

#
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
    static float3 SampleTextureFloat3(SurfaceParameters& surface, const SceneResource* scene, float2 uvs, uint textureIndex, bool sRGB, bool hasDifferentials)
    {
        if(textureIndex == InvalidIndex32)
            return float3::Zero_;

        TextureResource* textures = scene->textures;

        float3 sample;
        if(hasDifferentials) {
            sample = TextureFiltering::EWA(textures[textureIndex].data, uvs, surface.differentials.duvdx, surface.differentials.duvdy);
        }
        else {
            sample = TextureFiltering::Triangle(textures[textureIndex].data, 0, uvs);
        }

        if(sRGB) {
            sample = Math::SrgbToLinearPrecise(sample);
        }

        return sample;
    }

    //==============================================================================
    void CalculateSurfaceParams(const SceneResource* scene, const Ray& ray, float3 position, uint32 primitiveId, float2 barycentric, SurfaceParameters& surface)
    {
        uint32 i0 = scene->data->indices[3 * primitiveId + 0];
        uint32 i1 = scene->data->indices[3 * primitiveId + 1];
        uint32 i2 = scene->data->indices[3 * primitiveId + 2];

        const VertexAuxiliaryData& v0 = scene->data->vertexData[i0];
        const VertexAuxiliaryData& v1 = scene->data->vertexData[i1];
        const VertexAuxiliaryData& v2 = scene->data->vertexData[i2];

        Material* material = &scene->data->materials[v0.materialIndex];

        float3 p0 = float3(v0.px, v0.py, v0.pz);
        float3 p1 = float3(v1.px, v1.py, v1.pz);
        float3 p2 = float3(v2.px, v2.py, v2.pz);
        float3 n0 = float3(v0.nx, v0.ny, v0.nz);
        float3 n1 = float3(v1.nx, v1.ny, v1.nz);
        float3 n2 = float3(v2.nx, v2.ny, v2.nz);
        float3 t0 = float3(v0.tx, v0.ty, v0.tz);
        float3 t1 = float3(v1.tx, v1.ty, v1.tz);
        float3 t2 = float3(v2.tx, v2.ty, v2.tz);
        float3 b0 = Cross(n0, t0) * v0.bh;
        float3 b1 = Cross(n1, t1) * v1.bh;
        float3 b2 = Cross(n2, t2) * v2.bh;
        float2 uv0 = float2(v0.u, v0.v);
        float2 uv1 = float2(v1.u, v1.v);
        float2 uv2 = float2(v2.u, v2.v);

        float a0 = Saturate(1.0f - (barycentric.x + barycentric.y));
        float a1 = barycentric.x;
        float a2 = barycentric.y;

        float3 t = Normalize(a0 * t0 + a1 * t1 + a2 * t2);
        float3 b = Normalize(a0 * b0 + a1 * b1 + a2 * b2);
        float3 n = Normalize(a0 * n0 + a1 * n1 + a2 * n2);

        // -- adjust tangent frame to ensure it's facing in the direction the ray came from
        if(Dot(n, -ray.direction) < 0.0f) {
            t = -t;
            b = -b;
            n = -n;
        }

        // -- Calculate tangent space transforms
        surface.tangentToWorld = MakeFloat4x4(float4(t, 0.0f),
                                              float4(n, 0.0f),
                                              float4(b, 0.0f),
                                              float4(0.0f, 0.0f, 0.0f, 1.0f));
        surface.worldToTangent = MatrixTranspose(surface.tangentToWorld);

        surface.normal        = n;
        surface.position      = position;
        surface.materialFlags = material->flags;
        surface.metalness     = material->metalness;
        surface.specularColor = material->specularColor;
        surface.roughness     = material->roughness;

        bool canUseDifferentials = (material->flags & eHasTextures) && ray.hasDifferentials;
        bool preserveDifferentials = (material->flags | ePreserveRayDifferentials) && ray.hasDifferentials;
        
        if (canUseDifferentials || preserveDifferentials)  {
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
                surface.dpdu = (duv12.y * edge02 - duv02.y * edge12) * invDet;
                surface.dpdv = (-duv12.x * edge02 + duv02.x * edge12) * invDet;

                if(preserveDifferentials) {
                    surface.differentials.dndu = (duv12.y * dn02 - duv02.y * dn12) * invDet;
                    surface.differentials.dndv = (-duv12.x * dn02 + duv02.x * dn12) * invDet;
                }
            }
            if(degenerateUV || LengthSquared(Cross(surface.dpdu, surface.dpdv)) == 0.0f) {
                CoordinateSystem(Normalize(Cross(p2 - p0, p1 - p0)), &surface.dpdu, &surface.dpdv);
                surface.differentials.dndu = float3::Zero_;
                surface.differentials.dndv = float3::Zero_;
            }
        }

        if(canUseDifferentials) {
            CalculateSurfaceDifferentials(ray, surface.normal, position, surface.dpdu, surface.dpdv, surface.differentials);
        }

        float2 uvs = a0 * uv0 + a1 * uv1 + a2 * uv2;
        surface.emissive = SampleTextureFloat3(surface, scene, uvs, material->emissiveTextureIndex, false, ray.hasDifferentials);
        surface.albedo   = SampleTextureFloat3(surface, scene, uvs, material->albedoTextureIndex, false, ray.hasDifferentials);
    }
}