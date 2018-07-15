//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/BuildScene.h"
#include "BuildCommon/SceneBuildPipeline.h"
#include "BuildCommon/ImportMaterial.h"
#include "BuildCore/BuildContext.h"
#include "UtilityLib/Color.h"
#include "GeometryLib/AxisAlignedBox.h"
#include "MathLib/FloatFuncs.h"
#include "SystemLib/CheckedCast.h"
#include "SystemLib/MinMax.h"
#include "SystemLib/MemoryAllocation.h"
#include "SystemLib/CountOf.h"
#include "SystemLib/Logging.h"

namespace Selas
{
    //=============================================================================================================================
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

    //=============================================================================================================================
    static void AppendAndOffsetIndices(const CArray<uint32>& addend, uint32 offset, CArray <uint32>& indices)
    {
        uint addendCount = addend.Length();
        for(uint scan = 0; scan < addendCount; ++scan) {
            indices.Add(addend[scan] + offset);
        }
    }

    //=============================================================================================================================
    static void BuildMeshes(ImportedModel* imported, BuiltScene* built)
    {
        uint32 totalVertexCount = 0;
        uint32 totalIndexCount[eMeshIndexTypeCount] = { 0, 0, 0, 0 };

        for(uint scan = 0, count = imported->meshes.Length(); scan < count; ++scan) {
            ImportedMesh* mesh = imported->meshes[scan];
            Material* material = &built->materials[mesh->materialIndex];

            uint32 indexType = 0;

            bool alphaTested = material->flags & eAlphaTested;
            bool displaced = material->flags & eDisplacement;
            if(alphaTested & displaced)
                indexType = eMeshAlphaTestedDisplaced;
            else if(alphaTested)
                indexType = eMeshAlphaTested;
            else if(displaced)
                indexType = eMeshDisplaced;
            else
                indexType = eMeshStandard;

            totalVertexCount += mesh->positions.Length();
            totalIndexCount[indexType] += mesh->indices.Length();
        }

        for(uint scan = 0; scan < eMeshIndexTypeCount; ++scan) {
            built->indices[scan].Reserve(totalIndexCount[scan]);
        }

        built->positions.Reserve(totalVertexCount);
        built->normals.Reserve(totalVertexCount);
        built->tangents.Reserve(totalVertexCount);
        built->uvs.Reserve(totalVertexCount);
        built->materialIndices.Reserve(totalVertexCount);

        uint32 vertexOffset = 0;
        for(uint scan = 0, count = imported->meshes.Length(); scan < count; ++scan) {

            ImportedMesh* mesh = imported->meshes[scan];
            Material* material = &built->materials[mesh->materialIndex];

            BuiltMeshData meshData;
            meshData.indexCount = mesh->indices.Length();
            meshData.vertexCount = mesh->positions.Length();
            meshData.vertexOffset = vertexOffset;

            built->meshes.Add(meshData);

            bool alphaTested = material->flags & eAlphaTested;
            bool displaced = material->flags & eDisplacement;
            if(alphaTested & displaced)
                AppendAndOffsetIndices(mesh->indices, vertexOffset, built->indices[eMeshAlphaTestedDisplaced]);
            else if(alphaTested)
                AppendAndOffsetIndices(mesh->indices, vertexOffset, built->indices[eMeshAlphaTested]);
            else if(displaced)
                AppendAndOffsetIndices(mesh->indices, vertexOffset, built->indices[eMeshDisplaced]);
            else
                AppendAndOffsetIndices(mesh->indices, vertexOffset, built->indices[eMeshStandard]);
            
            built->positions.Append(mesh->positions);
            built->uvs.Append(mesh->uv0);

            for(uint i = 0; i < meshData.vertexCount; ++i) {

                float3 n = mesh->normals[i];
                float3 t = mesh->tangents[i];
                float3 b = mesh->bitangents[i];

                // -- Gram-Schmidt to make sure they are orthogonal
                t = t - Dot(n, t) * n;

                // -- calculate handedness of input bitangent
                float handedness = (Dot(Cross(n, t), b) < 0.0f) ? -1.0f : 1.0f;

                built->normals.Add(n);
                built->tangents.Add(float4(t, handedness));
                built->materialIndices.Add(mesh->materialIndex);
            }

            vertexOffset += meshData.vertexCount;
        }

        uint32 maxFaceIndices = Max(built->indices[eMeshDisplaced].Length() / 3,
                                    built->indices[eMeshAlphaTestedDisplaced].Length() / 3);
        built->faceIndexCounts.Resize(maxFaceIndices);
        for(uint scan = 0; scan < maxFaceIndices; ++scan) {
            built->faceIndexCounts[scan] = 3;
        }

        MakeInvalid(&built->aaBox);
        for(uint scan = 0, count = totalVertexCount; scan < count; ++scan) {
            IncludePosition(&built->aaBox, built->positions[scan]);
        }

        float3 center = 0.5f * (built->aaBox.max + built->aaBox.min);
        float radius = Length(built->aaBox.max - center);
        built->boundingSphere = float4(center, radius);
    }

    //=============================================================================================================================
    static uint32 AddTexture(BuiltScene* builtScene, const FilePathString& path)
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

    //=============================================================================================================================
    static Error ImportMaterials(BuildProcessorContext* context, cpointer prefix, ImportedModel* imported, BuiltScene* built)
    {
        built->materials.Resize(imported->materials.Length());
        for(uint scan = 0, count = imported->materials.Length(); scan < count; ++scan) {
            FilePathString materialfile;
            AssetFileUtils::ContentFilePath(prefix, imported->materials[scan].Ascii(), ".json", materialfile);
            
            ImportedMaterialData importedMaterialData;
            Error err = ImportMaterial(materialfile.Ascii(), &importedMaterialData);
            if(Failed_(err)) {
                WriteDebugInfo_("Failed to load material: %s", materialfile.Ascii());

                AssetFileUtils::ContentFilePath("Materials~Default.json", materialfile);
                ReturnError_(ImportMaterial(materialfile.Ascii(), &importedMaterialData));
            }

            context->AddFileDependency(materialfile.Ascii());

            Material& material = built->materials[scan];
            material = Material();

            if(importedMaterialData.alphaTested)
                material.flags |= eAlphaTested;
            if(importedMaterialData.invertDisplacement)
                material.flags |= eInvertDisplacement;

            uint32 shaderFlags = 0;
            DetermineShaderType(importedMaterialData, material.shader, shaderFlags);
            material.flags |= shaderFlags;

            if(StringUtil::Length(importedMaterialData.baseColorTexture.Ascii())) {
                material.baseColorTextureIndex = AddTexture(built, importedMaterialData.baseColorTexture);
            }
            if(StringUtil::Length(importedMaterialData.normalTexture.Ascii())) {
                material.normalTextureIndex = AddTexture(built, importedMaterialData.normalTexture);
            }

            for(uint scan = 0; scan < eMaterialPropertyCount; ++scan) {
                material.scalarAttributeValues[scan] = importedMaterialData.scalarAttributes[scan];
                
                const FilePathString& textureName = importedMaterialData.scalarAttributeTextures[scan];
                if(StringUtil::Length(textureName.Ascii())) {
                    material.scalarAttributeTextureIndices[scan] = AddTexture(built, textureName);
                }
            }

            if(material.scalarAttributeTextureIndices[eDisplacement] != InvalidIndex32) {
                material.flags |= eDisplacementEnabled;
            }
        }

        return Success_;
    }

    //=============================================================================================================================
    Error BuildScene(BuildProcessorContext* context, cpointer materialPrefix, ImportedModel* imported, BuiltScene* built)
    {
        ReturnError_(ImportMaterials(context, materialPrefix, imported, built));

        BuildMeshes(imported, built);

        built->camera = imported->camera;

        return Success_;
    }
}