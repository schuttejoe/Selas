//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/BuildScene.h"
#include "BuildCommon/SceneBuildPipeline.h"
#include "BuildCommon/ImportMaterial.h"
#include "BuildCore/BuildContext.h"
#include "UtilityLib/Color.h"
#include "GeometryLib/AxisAlignedBox.h"
#include "GeometryLib/CoordinateSystem.h"
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
        uint32 totalIndexCount = 0;
        uint32 totalFaceCount = 0;

        for(uint scan = 0, count = imported->meshes.Length(); scan < count; ++scan) {
            ImportedMesh* mesh = imported->meshes[scan];

            totalVertexCount += mesh->positions.Length();
            totalIndexCount += mesh->triindices.Length();
            totalIndexCount += mesh->quadindices.Length();

            totalFaceCount += mesh->triindices.Length() / 3;
            totalFaceCount += mesh->quadindices.Length() / 4;
        }

        built->indices.Reserve(totalIndexCount);
        built->positions.Reserve(totalVertexCount);
        built->normals.Reserve(totalVertexCount);
        built->tangents.Reserve(totalVertexCount);
        built->uvs.Reserve(totalVertexCount);
        built->faceIndexCounts.Reserve(totalFaceCount);

        uint32 vertexOffset = 0;
        uint32 indexOffset = 0;
        for(uint scan = 0, count = imported->meshes.Length(); scan < count; ++scan) {

            ImportedMesh* mesh = imported->meshes[scan];

            uint32 vertexCount = mesh->positions.Length();

            if(mesh->triindices.Length() > 0) {
                MeshMetaData meshData;
                meshData.indexCount = mesh->triindices.Length();
                meshData.indexOffset = indexOffset;
                meshData.vertexCount = vertexCount;
                meshData.vertexOffset = vertexOffset;
                meshData.materialIndex = mesh->materialIndex;
                meshData.indicesPerFace = 3;
                built->meshes.Add(meshData);

                AppendAndOffsetIndices(mesh->triindices, vertexOffset, built->indices);

                for(uint i = 0, faceCount = meshData.indexCount / 3; i < faceCount; ++i) {
                    built->faceIndexCounts.Add(3);
                }

                indexOffset += meshData.indexCount;
            }
            if(mesh->quadindices.Length() > 0) {
                MeshMetaData meshData;
                meshData.indexCount = mesh->quadindices.Length();
                meshData.indexOffset = indexOffset;
                meshData.vertexCount = vertexCount;
                meshData.vertexOffset = vertexOffset;
                meshData.materialIndex = mesh->materialIndex;
                meshData.indicesPerFace = 4;
                built->meshes.Add(meshData);

                AppendAndOffsetIndices(mesh->quadindices, vertexOffset, built->indices);

                for(uint i = 0, faceCount = meshData.indexCount / 4; i < faceCount; ++i) {
                    built->faceIndexCounts.Add(4);
                }

                indexOffset += meshData.indexCount;
            }
            
            built->positions.Append(mesh->positions);
            built->uvs.Append(mesh->uv0);

            for(uint i = 0; i < vertexCount; ++i) {

                float3 n = mesh->normals[i];
                float3 t;
                float3 b;
                if(i < mesh->tangents.Length() && i < mesh->bitangents.Length()) {
                    t = mesh->tangents[i];
                    b = mesh->bitangents[i];
                }
                else {
                    MakeOrthogonalCoordinateSystem(n, &t, &b);
                }

                // -- Gram-Schmidt to make sure they are orthogonal
                t = t - Dot(n, t) * n;

                // -- calculate handedness of input bitangent
                float handedness = (Dot(Cross(n, t), b) < 0.0f) ? -1.0f : 1.0f;

                built->normals.Add(n);
                built->tangents.Add(float4(t, handedness));
            }

            vertexOffset += vertexCount;
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
            material.baseColor = importedMaterialData.baseColor;

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

        built->backgroundIntensity = float3(0.6f, 0.6f, 0.6f);
        built->camera = imported->camera;

        return Success_;
    }
}