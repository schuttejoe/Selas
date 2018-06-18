//==============================================================================
// Joe Schutte
//==============================================================================

#include "SurfaceParameters.h"
#include "IntegratorContexts.h"

#include "TextureLib/TextureFiltering.h"
#include "TextureLib/TextureResource.h"
#include "SceneLib/SceneResource.h"
#include "GeometryLib/Ray.h"
#include "GeometryLib/CoordinateSystem.h"
#include "MathLib/FloatFuncs.h"
#include "MathLib/ColorSpace.h"

#define ForceNoMips_ true
#define EnableEWA_ true

namespace Selas
{
    //==============================================================================
    static float3 SampleTextureNormal(SurfaceParameters& surface, const SceneResource* scene, float2 uvs, uint textureIndex, bool hasDifferentials)
    {
        if(textureIndex == InvalidIndex32)
            return float3::ZAxis_;

        TextureResource* textures = scene->textures;
        if(textures[textureIndex].data->format != TextureResourceData::Float3) {
            Assert_(false);
            return float3::ZAxis_;
        }

        float3 sample;

        #if EnableDifferentials_        
            if(!ForceNoMips_ && hasDifferentials) {
                #if EnableEWA_
                    TextureFiltering::EWA(textures[textureIndex].data, uvs, surface.differentials.duvdx, surface.differentials.duvdy, sample);
                #else
                    TextureFiltering::Trilinear(textures[textureIndex].data, uvs, surface.differentials.duvdx, surface.differentials.duvdy, sample);
                #endif
            }
            else {
                TextureFiltering::Triangle(textures[textureIndex].data, 0, uvs, sample);
            }
        #else
            TextureFiltering::Triangle(textures[textureIndex].data, 0, uvs, sample);
        #endif

        return 2.0f * sample - 1.0f;
    }

    //==============================================================================
    static float SampleTextureOpacity(const SceneResource* scene, float2 uvs, uint textureIndex)
    {
        if(textureIndex == InvalidIndex32) {
            return 1.0f;
        }

        TextureResource* textures = scene->textures;
        if(textures[textureIndex].data->format != TextureResourceData::Float4) {
            return 1.0f;
        }

        float4 sample;
        TextureFiltering::Triangle(textures[textureIndex].data, 0, uvs, sample);

        return sample.w;
    }

    //==============================================================================
    template <typename Type_>
    static Type_ SampleTexture(SurfaceParameters& surface, const SceneResource* scene, float2 uvs, uint textureIndex, bool sRGB, bool hasDifferentials, Type_ defaultValue)
    {
        if(textureIndex == InvalidIndex32)
            return defaultValue;

        TextureResource* textures = scene->textures;

        Type_ sample;
        #if EnableDifferentials_
        if(!ForceNoMips_ && hasDifferentials) {
            #if EnableEWA_
            TextureFiltering::EWA(textures[textureIndex].data, uvs, surface.differentials.duvdx, surface.differentials.duvdy, sample);
            #else
            TextureFiltering::Trilinear(textures[textureIndex].data, uvs, surface.differentials.duvdx, surface.differentials.duvdy, sample);
            #endif
        }
        else {
            TextureFiltering::Triangle(textures[textureIndex].data, 0, uvs, sample);
        }
        #else
            TextureFiltering::Triangle(textures[textureIndex].data, 0, uvs, sample);
        #endif

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

        if(textures[textureIndex].data->format == TextureResourceData::Float) {
            return SampleTexture(surface, scene, uvs, textureIndex, sRGB, hasDifferentials, defaultValue);
        }
        else if(textures[textureIndex].data->format == TextureResourceData::Float2) {
            return SampleTexture(surface, scene, uvs, textureIndex, sRGB, hasDifferentials, float2(defaultValue, 0.0f).x);
        }
        else if(textures[textureIndex].data->format == TextureResourceData::Float3) {
            return SampleTexture(surface, scene, uvs, textureIndex, sRGB, hasDifferentials, float3(defaultValue, 0.0f, 0.0f).x);
        }
        else if(textures[textureIndex].data->format == TextureResourceData::Float4) {
            return SampleTexture(surface, scene, uvs, textureIndex, sRGB, hasDifferentials, float4(defaultValue, 0.0f, 0.0f, 0.0f).x);
        }

        Assert_(false);
        return 0.0f;
    }

    //==============================================================================
    static float3 SampleTextureFloat3(SurfaceParameters& surface, const SceneResource* scene, float2 uvs, uint textureIndex, bool sRGB, bool hasDifferentials, float defaultValue)
    {
        if(textureIndex == InvalidIndex32)
            return float3(defaultValue, defaultValue, defaultValue);

        TextureResource* textures = scene->textures;

        if(textures[textureIndex].data->format == TextureResourceData::Float) {
            float val;
            val = SampleTexture(surface, scene, uvs, textureIndex, sRGB, hasDifferentials, defaultValue);
            return float3(val, val, val);
        }
        else if(textures[textureIndex].data->format == TextureResourceData::Float3) {
            return SampleTexture(surface, scene, uvs, textureIndex, sRGB, hasDifferentials, float3(defaultValue, defaultValue, defaultValue));
        }
        else if(textures[textureIndex].data->format == TextureResourceData::Float4) {
            float4 val = SampleTexture(surface, scene, uvs, textureIndex, sRGB, hasDifferentials, float4(defaultValue, defaultValue, defaultValue, defaultValue));
            return val.XYZ();
        }

        Assert_(false);
        return 0.0f;
    }

    //==============================================================================
    static float4 SampleTextureFloat4(SurfaceParameters& surface, const SceneResource* scene, float2 uvs, uint textureIndex, bool sRGB, bool hasDifferentials, float defaultValue)
    {
        if(textureIndex == InvalidIndex32)
            return float4(defaultValue, defaultValue, defaultValue, defaultValue);

        TextureResource* textures = scene->textures;

        if(textures[textureIndex].data->format == TextureResourceData::Float) {
            float val = SampleTexture(surface, scene, uvs, textureIndex, sRGB, hasDifferentials, defaultValue);
            return float4(val, val, val, 1.0f);
        }
        else if(textures[textureIndex].data->format == TextureResourceData::Float3) {
            float3 value = SampleTexture(surface, scene, uvs, textureIndex, sRGB, hasDifferentials, float3(defaultValue, defaultValue, defaultValue));
            return float4(value, 1.0f);
        }
        else if(textures[textureIndex].data->format == TextureResourceData::Float4) {
            return SampleTexture(surface, scene, uvs, textureIndex, sRGB, hasDifferentials, float4(defaultValue, defaultValue, defaultValue, defaultValue));
        }

        Assert_(false);
        return 0.0f;
    }

    //==============================================================================
    static void CalculateSurfaceDifferentials(const HitParameters* __restrict hit, float3 n, float3 dpdu, float3 dpdv, SurfaceDifferentials& outputs)
    {
        {
            // -- See section 10.1.1 of PBRT 2nd edition

            float d = Dot(n, hit->position);
            float tx = -(Dot(n, hit->rxOrigin) - d) / Dot(n, hit->rxDirection);
            if(Math::IsInf(tx) || Math::IsNaN(tx))
                goto fail;
            float3 px = hit->rxOrigin + tx * hit->rxDirection;

            float ty = -(Dot(n, hit->ryOrigin) - d) / Dot(n, hit->ryDirection);
            if(Math::IsInf(ty) || Math::IsNaN(ty))
                goto fail;
            float3 py = hit->ryOrigin + ty * hit->ryDirection;

            outputs.dpdx = px - hit->position;
            outputs.dpdy = py - hit->position;

            // Initialize A, Bx, and By matrices for offset computation
            float2x2 A;
            float2 Bx;
            float2 By;

            if(Math::Absf(n.x) > Math::Absf(n.y) && Math::Absf(n.x) > Math::Absf(n.z)) {
                A.r0 = float2(dpdu.y, dpdv.y);
                A.r1 = float2(dpdu.z, dpdv.z);
                Bx = float2(px.y - hit->position.y, px.z - hit->position.z);
                By = float2(py.y - hit->position.y, py.z - hit->position.z);
            }
            else if(Math::Absf(n.y) > Math::Absf(n.z)) {
                A.r0 = float2(dpdu.x, dpdv.x);
                A.r1 = float2(dpdu.z, dpdv.z);
                Bx = float2(px.x - hit->position.x, px.z - hit->position.z);
                By = float2(py.x - hit->position.x, py.z - hit->position.z);
            }
            else {
                A.r0 = float2(dpdu.x, dpdv.x);
                A.r1 = float2(dpdu.y, dpdv.y);
                Bx = float2(px.x - hit->position.x, px.y - hit->position.y);
                By = float2(py.x - hit->position.x, py.y - hit->position.y);
            }

            if(!Matrix2x2::SolveLinearSystem(A, Bx, outputs.duvdx)) {
                outputs.duvdx = float2::Zero_;
            }

            if(!Matrix2x2::SolveLinearSystem(A, By, outputs.duvdy)) {
                outputs.duvdy = float2::Zero_;
            }
            return;
        }

        fail:
            outputs = SurfaceDifferentials();
    }

    //==============================================================================
    bool CalculateSurfaceParams(const KernelContext* context, const Ray& ray, const HitParameters* __restrict hit, SurfaceParameters& surface)
    {
        const SceneResource* scene = context->sceneData->scene;

        uint32 primitiveId = hit->primId;
        uint32 i0;
        uint32 i1;
        uint32 i2;
        
        if(hit->geomId == 0) {
            i0 = scene->data->indices[3 * primitiveId + 0];
            i1 = scene->data->indices[3 * primitiveId + 1];
            i2 = scene->data->indices[3 * primitiveId + 2];
        }
        else {
            i0 = scene->data->atIndices[3 * primitiveId + 0];
            i1 = scene->data->atIndices[3 * primitiveId + 1];
            i2 = scene->data->atIndices[3 * primitiveId + 2];
        }

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

        float a0 = Saturate(1.0f - (hit->baryCoords.x + hit->baryCoords.y));
        float a1 = hit->baryCoords.x;
        float a2 = hit->baryCoords.y;

        float3 t = Normalize(a0 * t0 + a1 * t1 + a2 * t2);
        float3 b = Normalize(a0 * b0 + a1 * b1 + a2 * b2);
        float3 n = Normalize(a0 * n0 + a1 * n1 + a2 * n2);

        if(Dot(n, -ray.direction) < 0.0f && ((material->flags & eTransparent) == 0)) {
            // -- we've hit inside of a non-transparent object. This is probably caused by floating point precision issues.
            return false;
        }

        bool rayHasDifferentials = hit->rxDirection.x != 0 || hit->rxDirection.y != 0;

        // -- Calculate tangent space transforms
        float3x3 tangentToWorld = MakeFloat3x3(t, n, b);
        surface.worldToTangent = MatrixTranspose(tangentToWorld);

        surface.geometricNormal = n;
        surface.position        = hit->position;
        surface.error           = hit->error;
        surface.materialFlags   = material->flags;
        
        bool canUseDifferentials = (material->flags & eHasTextures) && rayHasDifferentials;
        bool preserveDifferentials = (material->flags & ePreserveRayDifferentials) && rayHasDifferentials;
        
        #if EnableDifferentials_
            surface.rxDirection = hit->rxDirection;
            surface.ryDirection = hit->ryDirection;

            if (canUseDifferentials || preserveDifferentials)  {
                // Compute deltas for triangle partial derivatives
                float2 duv02 = uv0 - uv2;
                float2 duv12 = uv1 - uv2;
                float uvDeterminant     = duv02.x * duv12.y - duv02.y * duv12.x;

                bool degenerateUV = Math::Absf(uvDeterminant) < SmallFloatEpsilon_;
                if(!degenerateUV) {
                    float3 edge02 = p0 - p2;
                    float3 edge12 = p1 - p2;
                    float3 dn02 = n0 - n2;
                    float3 dn12 = n1 - n2;

                    float invUvDeterminant = 1 / uvDeterminant;
                    surface.dpdu = (duv12.y * edge02 - duv02.y * edge12) * invUvDeterminant;
                    surface.dpdv = (-duv12.x * edge02 + duv02.x * edge12) * invUvDeterminant;

                    if(preserveDifferentials) {
                        surface.differentials.dndu = ( duv12.y * dn02 - duv02.y * dn12) * invUvDeterminant;
                        surface.differentials.dndv = (-duv12.x * dn02 + duv02.x * dn12) * invUvDeterminant;
                    }
                }
                if(degenerateUV || LengthSquared(Cross(surface.dpdu, surface.dpdv)) == 0.0f) {
                    MakeOrthogonalCoordinateSystem(Normalize(Cross(p2 - p0, p1 - p0)), &surface.dpdu, &surface.dpdv);
                    surface.differentials.dndu = float3::Zero_;
                    surface.differentials.dndv = float3::Zero_;
                }
            }

            if(canUseDifferentials) {
                CalculateSurfaceDifferentials(hit, surface.geometricNormal, surface.dpdu, surface.dpdv, surface.differentials);
            }
        #endif

        float2 uvs = a0 * uv0 + a1 * uv1 + a2 * uv2;
        surface.albedo        = material->albedo * SampleTextureFloat3(surface, scene, uvs, material->albedoTextureIndex, false, rayHasDifferentials, 1.0f);
        surface.specularColor = SampleTextureFloat3(surface, scene, uvs, material->specularTextureIndex, false, rayHasDifferentials, 0.01f);
        surface.roughness     = material->roughness * SampleTextureFloat(surface, scene, uvs, material->roughnessTextureIndex, false, rayHasDifferentials, 1.0f);
        surface.metalness     = material->metalness * SampleTextureFloat(surface, scene, uvs, material->metalnessTextureIndex, false, rayHasDifferentials, 1.0f);

        surface.shader = material->shader;
        surface.view = -ray.direction;
        surface.currentIor = (Dot(-ray.direction, surface.geometricNormal) < 0.0f) ? material->ior : 1.0f;
        surface.exitIor = (Dot(-ray.direction, surface.geometricNormal) < 0.0f) ? 1.0f : material->ior;

        float3x3 normalToWorld = MakeFloat3x3(t, -b, n);
        float3 perturbNormal = SampleTextureNormal(surface, scene, uvs, material->normalTextureIndex, rayHasDifferentials);
        surface.perturbedNormal = Normalize(MatrixMultiply(perturbNormal, normalToWorld));

        return true;
    }

    //==============================================================================
    bool CalculatePassesAlphaTest(const SceneResource* scene, uint32 primitiveId, float2 baryCoords)
    {
        uint32 i0 = scene->data->atIndices[3 * primitiveId + 0];
        uint32 i1 = scene->data->atIndices[3 * primitiveId + 1];
        uint32 i2 = scene->data->atIndices[3 * primitiveId + 2];

        const VertexAuxiliaryData& v0 = scene->data->vertexData[i0];
        const VertexAuxiliaryData& v1 = scene->data->vertexData[i1];
        const VertexAuxiliaryData& v2 = scene->data->vertexData[i2];

        Material* material = &scene->data->materials[v0.materialIndex];
        Assert_(material->flags & eAlphaTested);

        float2 uv0 = float2(v0.u, v0.v);
        float2 uv1 = float2(v1.u, v1.v);
        float2 uv2 = float2(v2.u, v2.v);

        float a0 = Saturate(1.0f - (baryCoords.x + baryCoords.y));
        float a1 = baryCoords.x;
        float a2 = baryCoords.y;

        static const float kAlphaTestCutoff = 0.5f;

        float2 uvs = a0 * uv0 + a1 * uv1 + a2 * uv2;
        return SampleTextureOpacity(scene, uvs, material->albedoTextureIndex) > kAlphaTestCutoff;
    }

    //==============================================================================
    float3 OffsetRayOrigin(const SurfaceParameters& surface, float3 direction, float biasScale)
    {
        float directionOffset = Dot(direction, surface.geometricNormal) < 0.0f ? -1.0f : 1.0f;
        float3 offset = directionOffset * surface.error * biasScale * surface.geometricNormal;
        return surface.position + offset;
    }

    //==============================================================================
    float3 OffsetRayOrigin(const SurfaceParameters& surface, float3 direction, float biasScale, float& signedBiasDistance)
    {
        float directionOffset = Dot(direction, surface.geometricNormal) < 0.0f ? -1.0f : 1.0f;
        signedBiasDistance = directionOffset * surface.error * biasScale;
        float3 offset = signedBiasDistance * surface.geometricNormal;
        return surface.position + offset;
    }
}