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

#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>

#define ForceNoMips_ true
#define EnableEWA_ true

namespace Selas
{
    //==============================================================================
    static float3 SampleTextureNormal(const SceneResource* scene, float2 uvs, uint textureIndex)
    {
        if(textureIndex == InvalidIndex32)
            return float3::ZAxis_;

        TextureResource* textures = scene->textures;
        if(textures[textureIndex].data->format != TextureResourceData::Float3) {
            Assert_(false);
            return float3::ZAxis_;
        }

        float3 sample;
        TextureFiltering::Triangle(textures[textureIndex].data, 0, uvs, sample);

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
    static Type_ SampleTexture(const SceneResource* scene, float2 uvs, uint textureIndex, bool sRGB, Type_ defaultValue)
    {
        if(textureIndex == InvalidIndex32)
            return defaultValue;

        TextureResource* textures = scene->textures;

        Type_ sample;
        TextureFiltering::Triangle(textures[textureIndex].data, 0, uvs, sample);

        if(sRGB) {
            sample = Math::SrgbToLinearPrecise(sample);
        }

        return sample;
    }

    //==============================================================================
    static float SampleTextureFloat(const SceneResource* scene, float2 uvs, uint textureIndex, bool sRGB, float defaultValue)
    {
        if(textureIndex == InvalidIndex32)
            return defaultValue;

        TextureResource* textures = scene->textures;

        if(textures[textureIndex].data->format == TextureResourceData::Float) {
            return SampleTexture(scene, uvs, textureIndex, sRGB, defaultValue);
        }
        else if(textures[textureIndex].data->format == TextureResourceData::Float2) {
            return SampleTexture(scene, uvs, textureIndex, sRGB, float2(defaultValue, 0.0f)).x;
        }
        else if(textures[textureIndex].data->format == TextureResourceData::Float3) {
            return SampleTexture(scene, uvs, textureIndex, sRGB, float3(defaultValue, 0.0f, 0.0f)).x;
        }
        else if(textures[textureIndex].data->format == TextureResourceData::Float4) {
            return SampleTexture(scene, uvs, textureIndex, sRGB, float4(defaultValue, 0.0f, 0.0f, 0.0f)).x;
        }

        Assert_(false);
        return 0.0f;
    }

    //==============================================================================
    static float3 SampleTextureFloat3(const SceneResource* scene, float2 uvs, uint textureIndex, bool sRGB, float defaultValue)
    {
        if(textureIndex == InvalidIndex32)
            return float3(defaultValue, defaultValue, defaultValue);

        TextureResource* textures = scene->textures;

        if(textures[textureIndex].data->format == TextureResourceData::Float) {
            float val;
            val = SampleTexture(scene, uvs, textureIndex, sRGB, defaultValue);
            return float3(val, val, val);
        }
        else if(textures[textureIndex].data->format == TextureResourceData::Float3) {
            return SampleTexture(scene, uvs, textureIndex, sRGB, float3(defaultValue, defaultValue, defaultValue));
        }
        else if(textures[textureIndex].data->format == TextureResourceData::Float4) {
            float4 val = SampleTexture(scene, uvs, textureIndex, sRGB, float4(defaultValue, defaultValue, defaultValue, defaultValue));
            return val.XYZ();
        }

        Assert_(false);
        return 0.0f;
    }

    //==============================================================================
    static float4 SampleTextureFloat4(const SceneResource* scene, float2 uvs, uint textureIndex, bool sRGB, float defaultValue)
    {
        if(textureIndex == InvalidIndex32)
            return float4(defaultValue, defaultValue, defaultValue, defaultValue);

        TextureResource* textures = scene->textures;

        if(textures[textureIndex].data->format == TextureResourceData::Float) {
            float val = SampleTexture(scene, uvs, textureIndex, sRGB, defaultValue);
            return float4(val, val, val, 1.0f);
        }
        else if(textures[textureIndex].data->format == TextureResourceData::Float3) {
            float3 value = SampleTexture(scene, uvs, textureIndex, sRGB, float3(defaultValue, defaultValue, defaultValue));
            return float4(value, 1.0f);
        }
        else if(textures[textureIndex].data->format == TextureResourceData::Float4) {
            return SampleTexture(scene, uvs, textureIndex, sRGB, float4(defaultValue, defaultValue, defaultValue, defaultValue));
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
    static Material* GetSurfaceMaterial(const SceneResource* scene, uint32 geomId, uint32 primId)
    {
        Assert_(geomId < eMeshIndexTypeCount);
        Assert_(primId < scene->data->indexCounts[geomId]);
        uint32 i0 = scene->geometry->indices[geomId][3 * primId + 0];
        uint32 materialIndex = scene->geometry->materialIndices[i0];

        return &scene->data->materials[materialIndex];
    }

    //==============================================================================
    bool CalculateSurfaceParams(const GIIntegrationContext* context, const Ray& ray, const HitParameters* __restrict hit, SurfaceParameters& surface)
    {
        const SceneResource* scene = context->sceneData->scene;

        Material* material = GetSurfaceMaterial(scene, hit->geomId, hit->primId);

        Align_(16) float3 normal;
        rtcInterpolate0((RTCGeometry)scene->rtcGeometries[hit->geomId], hit->primId, hit->baryCoords.x, hit->baryCoords.y, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, &normal.x, 3);
        Align_(16) float4 tangent;
        rtcInterpolate0((RTCGeometry)scene->rtcGeometries[hit->geomId], hit->primId, hit->baryCoords.x, hit->baryCoords.y, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, &tangent.x, 4);
        Align_(16) float2 uvs;
        rtcInterpolate0((RTCGeometry)scene->rtcGeometries[hit->geomId], hit->primId, hit->baryCoords.x, hit->baryCoords.y, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 2, &uvs.x, 2);

        float3 n = Normalize(normal);
        float3 t = tangent.XYZ();
        float3 b = Cross(normal, t) * tangent.w;

        if(Dot(n, -ray.direction) < 0.0f && ((material->flags & eTransparent) == 0)) {
            // -- we've hit inside of a non-transparent object. This is probably caused by floating point precision issues.
            return false;
        }

        // -- Calculate tangent space transforms
        float3x3 tangentToWorld = MakeFloat3x3(t, n, b);
        surface.worldToTangent = MatrixTranspose(tangentToWorld);

        surface.geometricNormal = n;
        surface.position        = hit->position;
        surface.error           = hit->error;
        surface.materialFlags   = material->flags;
       
        surface.albedo        = material->albedo * SampleTextureFloat3(scene, uvs, material->albedoTextureIndex, false, 1.0f);
        surface.specularColor = SampleTextureFloat3(scene, uvs, material->specularTextureIndex, false, 0.01f);
        surface.roughness     = material->roughness * SampleTextureFloat(scene, uvs, material->roughnessTextureIndex, false, 1.0f);
        surface.metalness     = material->metalness * SampleTextureFloat(scene, uvs, material->metalnessTextureIndex, false, 1.0f);

        surface.shader = material->shader;
        surface.view = -ray.direction;
        surface.currentIor = (Dot(-ray.direction, surface.geometricNormal) < 0.0f) ? material->ior : 1.0f;
        surface.exitIor = (Dot(-ray.direction, surface.geometricNormal) < 0.0f) ? 1.0f : material->ior;

        float3x3 normalToWorld = MakeFloat3x3(t, -b, n);
        float3 perturbNormal = SampleTextureNormal(scene, uvs, material->normalTextureIndex);
        surface.perturbedNormal = Normalize(MatrixMultiply(perturbNormal, normalToWorld));

        return true;
    }

    //==============================================================================
    bool CalculatePassesAlphaTest(const SceneResource* scene, uint32 geomId, uint32 primId, float2 baryCoords)
    {
        Material* material = GetSurfaceMaterial(scene, geomId, primId);

        Align_(16) float2 uvs;
        rtcInterpolate0((RTCGeometry)scene->rtcGeometries[geomId], primId, baryCoords.x, baryCoords.y, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 2, &uvs.x, 2);

        static const float kAlphaTestCutoff = 0.5f;
        return SampleTextureOpacity(scene, uvs, material->albedoTextureIndex) > kAlphaTestCutoff;
    }

    //==============================================================================
    float CalculateDisplacement(const SceneResource* scene, uint32 geomId, uint32 primId, float2 uvs)
    {
        Material* material = GetSurfaceMaterial(scene, geomId, primId);
        float displacement = SampleTextureFloat(scene, uvs, material->displacementTextureIndex, false, 0.0f);
        if(material->flags & eInvertDisplacement) {
            displacement = 1.0f - displacement;
        }

        return material->displacementScale * displacement;
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