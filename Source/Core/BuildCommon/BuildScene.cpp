//==============================================================================
// Joe Schutte
//==============================================================================

#include <BuildCommon/BuildScene.h>
#include <BuildCommon/SceneBuildPipeline.h>
#include <BuildCommon/ImportMaterial.h>
#include <SystemLib/CheckedCast.h>
#include <SystemLib/MinMax.h>
#include <SystemLib/MemoryAllocation.h>
#include <MathLib/FloatFuncs.h>
#include <UtilityLib/Color.h>

namespace Shooty
{
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

            if(StringUtil::Length(importedMaterialData.emissive.Ascii())) {
                material.flags |= eHasTextures;
                material.emissiveTextureIndex = AddTexture(built, importedMaterialData.emissive);
            }
            if(StringUtil::Length(importedMaterialData.albedo.Ascii())) {
                material.flags |= eHasReflectance | eHasTextures;
                material.albedoTextureIndex = AddTexture(built, importedMaterialData.albedo);
                material.metalness = 0.1f;
            }
            if(StringUtil::Length(importedMaterialData.height.Ascii())) {
                material.flags |= eHasTextures;
                material.heightTextureIndex = AddTexture(built, importedMaterialData.height);
            }
            if(StringUtil::Length(importedMaterialData.roughness.Ascii())) {
                material.flags |= eHasReflectance | eHasTextures;
                material.roughnessTextureIndex = AddTexture(built, importedMaterialData.roughness);
            }
            if(StringUtil::Length(importedMaterialData.normal.Ascii())) {
                material.flags |= eHasTextures;
                material.normalTextureIndex = AddTexture(built, importedMaterialData.normal);
            }
            if(StringUtil::Length(importedMaterialData.specular.Ascii())) {
                material.flags |= eHasTextures | ePreserveRayDifferentials;
                material.specularTextureIndex = AddTexture(built, importedMaterialData.specular);
                material.metalness = 1.0f;
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