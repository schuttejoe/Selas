//==============================================================================
// Joe Schutte
//==============================================================================

#include "SurfaceParameters.h"
#include <TextureLib/TextureFiltering.h>
#include <TextureLib/TextureResource.h>
#include <SceneLib/SceneResource.h>
#include <GeometryLib/Ray.h>
#include <GeometryLib/CoordinateSystem.h>
#include <MathLib/FloatFuncs.h>
#include <MathLib/ColorSpace.h>

namespace Shooty
{
    //==============================================================================
    static float3 SampleTextureNormal(SurfaceParameters& surface, const SceneResource* scene, float2 uvs, uint textureIndex, bool hasDifferentials)
    {
        if(textureIndex == InvalidIndex32)
            return float3::ZAxis_;

        TextureResource* textures = scene->textures;

        float3 sample;
        //if(hasDifferentials) {
        //    //sample = TextureFiltering::EWAFloat3(textures[textureIndex].data, uvs, surface.differentials.duvdx, surface.differentials.duvdy);
        //}
        //else {
            sample = TextureFiltering::TriangleFloat3(textures[textureIndex].data, 0, uvs);
        //}

        return 2.0f * sample - 1.0f;
    }

    //==============================================================================
    static float3 SampleTextureFloat3(SurfaceParameters& surface, const SceneResource* scene, float2 uvs, uint textureIndex, bool sRGB, bool hasDifferentials, float3 defaultValue)
    {
        if(textureIndex == InvalidIndex32)
            return defaultValue;

        TextureResource* textures = scene->textures;

        float3 sample;
        //if(hasDifferentials) {
        //    sample = TextureFiltering::EWAFloat3(textures[textureIndex].data, uvs, surface.differentials.duvdx, surface.differentials.duvdy);
        //}
        //else {
            sample = TextureFiltering::TriangleFloat3(textures[textureIndex].data, 0, uvs);
        //}

        if(sRGB) {
            sample = Math::SrgbToLinearPrecise(sample);
        }

        return sample;
    }

    //==============================================================================
    static float SampleTextureFloat(SurfaceParameters& surface, const SceneResource* scene, float2 uvs, uint textureIndex, bool sRGB, bool hasDifferentials, float defaultValue)
    {
        if(textureIndex == InvalidIndex32)
            return defaultValue;

        TextureResource* textures = scene->textures;

        float sample;
        //if(hasDifferentials) {
        //    //sample = TextureFiltering::EWAFloat(textures[textureIndex].data, uvs, surface.differentials.duvdx, surface.differentials.duvdy);
        //}
        //else {
            sample = TextureFiltering::TriangleFloat(textures[textureIndex].data, 0, uvs);
        //}

        if(sRGB) {
            sample = Math::SrgbToLinearPrecise(sample);
        }

        return sample;
    }

    //==============================================================================
    bool CalculateSurfaceParams(const SceneResource* scene, const Ray& ray, float3 position, float error, uint32 primitiveId, float2 barycentric, SurfaceParameters& surface)
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

        if(Dot(n, -ray.direction) < 0.0f && ((material->flags & eTransparent) == 0)) {
            // -- we've hit inside of a non-transparent object. This is probably caused by floating point precision issues.
            return false;
        }

        // -- Calculate tangent space transforms
        surface.tangentToWorld = MakeFloat3x3(t, n, b);
        surface.worldToTangent = MatrixTranspose(surface.tangentToWorld);

        surface.geometricNormal = n;
        surface.position        = position;
        surface.error           = error;
        surface.materialFlags   = material->flags;
        
        bool canUseDifferentials = (material->flags & eHasTextures) && ray.hasDifferentials;
        bool preserveDifferentials = (material->flags & ePreserveRayDifferentials) && ray.hasDifferentials;
        
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
                MakeOrthogonalCoordinateSystem(Normalize(Cross(p2 - p0, p1 - p0)), &surface.dpdu, &surface.dpdv);
                surface.differentials.dndu = float3::Zero_;
                surface.differentials.dndv = float3::Zero_;
            }
        }

        if(canUseDifferentials) {
            CalculateSurfaceDifferentials(ray, surface.geometricNormal, position, surface.dpdu, surface.dpdv, surface.differentials);
        }

        float2 uvs = a0 * uv0 + a1 * uv1 + a2 * uv2;
        surface.emissive = SampleTextureFloat3(surface, scene, uvs, material->emissiveTextureIndex, false, ray.hasDifferentials, float3::Zero_);
        surface.albedo = SampleTextureFloat3(surface, scene, uvs, material->albedoTextureIndex, false, ray.hasDifferentials, float3::Zero_);
        surface.specularColor = SampleTextureFloat3(surface, scene, uvs, material->specularTextureIndex, false, ray.hasDifferentials, surface.albedo);
        surface.roughness = SampleTextureFloat(surface, scene, uvs, material->roughnessTextureIndex, false, ray.hasDifferentials, 1.0f);
        surface.metalness = material->metalness * SampleTextureFloat(surface, scene, uvs, material->metalnessTextureIndex, false, ray.hasDifferentials, 1.0f);

        surface.shader = material->shader;
        surface.ior = material->ior;

        float3x3 normalToWorld = MakeFloat3x3(t, -b, n);
        float3 perturbNormal = SampleTextureNormal(surface, scene, uvs, material->normalTextureIndex, ray.hasDifferentials);
        surface.perturbedNormal = Normalize(MatrixMultiply(perturbNormal, normalToWorld));

        return true;
    }

    //==============================================================================
    float3 OffsetRayOrigin(const SurfaceParameters& surface, float3 direction, float biasScale)
    {
        float offsetDirection = Dot(direction, surface.geometricNormal) < 0.0f ? -1.0f : 1.0f;
        float3 offset = offsetDirection * surface.error * biasScale * surface.geometricNormal;
        return surface.position + offset;
    }
}