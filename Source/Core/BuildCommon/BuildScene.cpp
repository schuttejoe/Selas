//==============================================================================
// Joe Schutte
//==============================================================================

#include <BuildCommon/BuildScene.h>
#include <BuildCommon/SceneBuildPipeline.h>
#include <BuildCommon/ImportMaterial.h>
#include <UtilityLib/Color.h>
#include <GeometryLib/AxisAlignedBox.h>
#include <MathLib/FloatFuncs.h>
#include <SystemLib/CheckedCast.h>
#include <SystemLib/MinMax.h>
#include <SystemLib/MemoryAllocation.h>
#include <SystemLib/CountOf.h>

namespace Shooty
{
    //==============================================================================
    static void DetermineShaderType(ImportedMaterialData& material, eMaterialShader& shader, uint32& shaderFlags)
    {
        static const char* shaderNames[] = {
            "Disney",
            "TransparentGGX"
        };
        static_assert(CountOf_(shaderNames) == eShaderCount, "Missing shader name");

        shaderFlags = 0;
        shader = eDisney;
        for(uint scan = 0; scan < eShaderCount; ++scan) {
            if(StringUtil::EqualsIgnoreCase(shaderNames[scan], material.shaderName.Ascii())) {
                shader = (eMaterialShader)scan;
            }
        }

        if(shader == eTransparentGgx) {
            shaderFlags |= eTransparent;
        }
    }

    //==============================================================================
    static void AppendAndOffsetIndices(const CArray<uint32>& addend, uint32 offset, CArray <uint32>& indices)
    {
        uint addendCount = addend.Length();
        uint32 newTotal = (uint32)(indices.Length() + addendCount);

        indices.Reserve(newTotal);

        for(uint scan = 0; scan < addendCount; ++scan) {
            indices.Add(addend[scan] + offset);
        }
    }

    //==============================================================================
    static void BuildMeshes(ImportedModel* imported, BuiltScene* built)
    {
        uint32 totalIndexCount = 0;
        uint32 totalVertexCount = 0;

        for(uint scan = 0, count = imported->meshes.Length(); scan < count; ++scan) {

            ImportedMesh* mesh = imported->meshes[scan];

            BuiltMeshData meshData;
            meshData.indexCount = mesh->indices.Length();
            meshData.vertexCount = mesh->positions.Length();
            meshData.indexOffset = totalIndexCount;
            meshData.vertexOffset = totalVertexCount;

            built->meshes.Add(meshData);
            AppendAndOffsetIndices(mesh->indices, totalVertexCount, built->indices);
            built->positions.Append(mesh->positions);

            built->vertexData.Reserve(built->vertexData.Length() + meshData.vertexCount);
            for(uint i = 0; i < meshData.vertexCount; ++i) {

                float3 n = mesh->normals[i];
                float3 t = mesh->tangents[i];
                float3 b = mesh->bitangents[i];

                // -- Gram-Schmidt to make sure they are orthogonal
                t = t - Dot(n, t) * n;

                // -- calculate handedness of input bitangent
                float handedness = (Dot(Cross(n, t), b) < 0.0f) ? -1.0f : 1.0f;

                VertexAuxiliaryData vertexData;
                vertexData.px            = mesh->positions[i].x;
                vertexData.py            = mesh->positions[i].y;
                vertexData.pz            = mesh->positions[i].z;
                vertexData.nx            = n.x;
                vertexData.ny            = n.y;
                vertexData.nz            = n.z;
                vertexData.tx            = t.x;
                vertexData.ty            = t.y;
                vertexData.tz            = t.z;
                vertexData.bh            = handedness;
                vertexData.u             = mesh->uv0[i].x;
                vertexData.v             = mesh->uv0[i].y;
                vertexData.materialIndex = mesh->materialIndex;

                built->vertexData.Add(vertexData);
            }

            totalIndexCount += meshData.indexCount;
            totalVertexCount += meshData.vertexCount;
        }

        MakeInvalid(&built->aaBox);
        for(uint scan = 0, count = totalVertexCount; scan < count; ++scan) {
            IncludePosition(&built->aaBox, built->positions[scan]);
        }

        float3 center = 0.5f * (built->aaBox.max + built->aaBox.min);
        float radius = Length(built->aaBox.max - center);
        built->boundingSphere = float4(center, radius);
    }

    //==============================================================================
    static uint32 AddTexture(BuiltScene* builtScene, const FixedString256& path)
    {
        // JSTODO - Implement a hash set
        for(uint scan = 0, count = builtScene->textures.Length(); scan < count; ++scan) {
            if(StringUtil::EqualsIgnoreCase(builtScene->textures[scan].Ascii(), path.Ascii())) {
                return (uint32)scan;
            }
        }

        builtScene->textures.Add(path);
        return (uint32)builtScene->textures.Length() - 1;
    }

    //==============================================================================
    static bool ImportMaterials(ImportedModel* imported, BuiltScene* built)
    {
        built->materials.Resize(imported->materials.Length());
        for(uint scan = 0, count = imported->materials.Length(); scan < count; ++scan) {
            ImportedMaterialData importedMaterialData;
            ReturnFailure_(ImportMaterial(imported->materials[scan].Ascii(), &importedMaterialData));

            Material& material = built->materials[scan];
            material = Material();

            uint32 shaderFlags = 0;
            DetermineShaderType(importedMaterialData, material.shader, shaderFlags);

            material.flags |= shaderFlags;
            material.metalness = importedMaterialData.metalness;
            material.roughness = importedMaterialData.roughness;
            material.albedo    = importedMaterialData.albedo;
            material.ior = importedMaterialData.ior;

            if(StringUtil::Length(importedMaterialData.albedoTextureName.Ascii())) {
                material.flags |= eHasTextures;
                material.albedoTextureIndex = AddTexture(built, importedMaterialData.albedoTextureName);
            }
            if(StringUtil::Length(importedMaterialData.heightTextureName.Ascii())) {
                material.flags |= eHasTextures;
                material.heightTextureIndex = AddTexture(built, importedMaterialData.heightTextureName);
            }
            if(StringUtil::Length(importedMaterialData.roughnessTextureName.Ascii())) {
                material.flags |= eHasTextures;
                material.roughnessTextureIndex = AddTexture(built, importedMaterialData.roughnessTextureName);
            }
            if(StringUtil::Length(importedMaterialData.normalTextureName.Ascii())) {
                material.flags |= eHasTextures;
                material.normalTextureIndex = AddTexture(built, importedMaterialData.normalTextureName);
            }
            if(StringUtil::Length(importedMaterialData.specularTextureName.Ascii())) {
                material.flags |= eHasTextures | ePreserveRayDifferentials;
                material.specularTextureIndex = AddTexture(built, importedMaterialData.specularTextureName);
            }
            if(StringUtil::Length(importedMaterialData.metalnessTextureName.Ascii())) {
                material.flags |= eHasTextures | ePreserveRayDifferentials;
                material.metalnessTextureIndex = AddTexture(built, importedMaterialData.metalnessTextureName);
            }
        }

        return true;
    }

    //==============================================================================
    bool BuildScene(ImportedModel* imported, BuiltScene* built)
    {
        ReturnFailure_(ImportMaterials(imported, built));

        BuildMeshes(imported, built);

        built->camera = imported->camera;

        return true;
    }
}