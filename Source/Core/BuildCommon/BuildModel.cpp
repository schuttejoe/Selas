//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/BuildModel.h"
#include "BuildCommon/ModelBuildPipeline.h"
#include "BuildCommon/ImportMaterial.h"
#include "BuildCore/BuildContext.h"
#include "UtilityLib/Color.h"
#include "UtilityLib/QuickSort.h"
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
    static void DetermineShaderType(ImportedMaterialData& material, eMaterialShader& shader)
    {
        static const char* shaderNames[] = {
            "DisneyThin",
            "DisneySolid"
        };
        static_assert(CountOf_(shaderNames) == eShaderCount, "Missing shader name");

        shader = eDisneySolid;
        for(uint scan = 0; scan < eShaderCount; ++scan) {
            if(StringUtil::EqualsIgnoreCase(shaderNames[scan], material.shaderName.Ascii())) {
                shader = (eMaterialShader)scan;
            }
        }
    }

    //=============================================================================================================================
    static void AppendAndOffsetIndices(const CArray<uint32>& addend, uint32 offset, CArray <uint32>& indices)
    {
        uint addendCount = addend.Count();
        for(uint scan = 0; scan < addendCount; ++scan) {
            indices.Add(addend[scan] + offset);
        }
    }

    //=============================================================================================================================
    static void BuildMeshes(ImportedModel* imported, BuiltModel* built)
    {
        uint32 totalVertexCount = 0;
        uint32 totalIndexCount = 0;
        uint32 totalFaceCount = 0;

        for(uint scan = 0, count = imported->meshes.Count(); scan < count; ++scan) {
            ImportedMesh* mesh = imported->meshes[scan];

            totalVertexCount += mesh->positions.Count();
            totalIndexCount += mesh->triindices.Count();
            totalIndexCount += mesh->quadindices.Count();

            totalFaceCount += mesh->triindices.Count() / 3;
            totalFaceCount += mesh->quadindices.Count() / 4;
        }

        built->indices.Reserve(totalIndexCount);
        built->positions.Reserve(totalVertexCount);
        built->normals.Reserve(totalVertexCount);
        built->tangents.Reserve(totalVertexCount);
        built->uvs.Reserve(totalVertexCount);
        built->faceIndexCounts.Reserve(totalFaceCount);

        uint32 vertexOffset = 0;
        uint32 indexOffset = 0;
        for(uint scan = 0, count = imported->meshes.Count(); scan < count; ++scan) {

            ImportedMesh* mesh = imported->meshes[scan];

            uint32 vertexCount = mesh->positions.Count();

            if(mesh->triindices.Count() > 0) {
                MeshMetaData meshData;
                meshData.indexCount = mesh->triindices.Count();
                meshData.indexOffset = indexOffset;
                meshData.vertexCount = vertexCount;
                meshData.vertexOffset = vertexOffset;
                meshData.materialHash = mesh->materialHash;
                meshData.indicesPerFace = 3;
                built->meshes.Add(meshData);

                AppendAndOffsetIndices(mesh->triindices, vertexOffset, built->indices);

                for(uint i = 0, faceCount = meshData.indexCount / 3; i < faceCount; ++i) {
                    built->faceIndexCounts.Add(3);
                }

                indexOffset += meshData.indexCount;
            }
            if(mesh->quadindices.Count() > 0) {
                MeshMetaData meshData;
                meshData.indexCount = mesh->quadindices.Count();
                meshData.indexOffset = indexOffset;
                meshData.vertexCount = vertexCount;
                meshData.vertexOffset = vertexOffset;
                meshData.materialHash = mesh->materialHash;
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
                if(i < mesh->tangents.Count() && i < mesh->bitangents.Count()) {
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
    static uint32 AddTexture(BuiltModel* builtScene, const FilePathString& path)
    {
        // JSTODO - Implement a hash set
        for(uint scan = 0, count = builtScene->textures.Count(); scan < count; ++scan) {
            if(StringUtil::EqualsIgnoreCase(builtScene->textures[scan].Ascii(), path.Ascii())) {
                return (uint32)scan;
            }
        }

        builtScene->textures.Add(path);
        return (uint32)builtScene->textures.Count() - 1;
    }

    //=============================================================================================================================
    static Error ImportMaterials(BuildProcessorContext* context, cpointer prefix, ImportedModel* imported, BuiltModel* built)
    {
        built->materials.Reserve(imported->materials.Count());
        for(uint scan = 0, count = imported->materials.Count(); scan < count; ++scan) {
            FilePathString materialfile;
            AssetFileUtils::ContentFilePath(prefix, imported->materials[scan].Ascii(), ".json", materialfile);
            
            ImportedMaterialData importedMaterialData;
            Error err = ImportMaterial(materialfile.Ascii(), &importedMaterialData);
            if(Failed_(err)) {
                WriteDebugInfo_("Failed to load material: %s", materialfile.Ascii());
                continue;
            }

            context->AddFileDependency(materialfile.Ascii());

            built->materialHashes.Add(imported->materialHashes[scan]);
            Material& material = built->materials.Add();
            material = Material();

            if(importedMaterialData.alphaTested)
                material.flags |= eAlphaTested;
            if(importedMaterialData.invertDisplacement)
                material.flags |= eInvertDisplacement;

            DetermineShaderType(importedMaterialData, material.shader);
            material.baseColor = importedMaterialData.baseColor;
            material.transmittanceColor = importedMaterialData.transmittanceColor;

            if(StringUtil::Length(importedMaterialData.baseColorTexture.Ascii())) {
                material.baseColorTextureIndex = AddTexture(built, importedMaterialData.baseColorTexture);
            }
            if(StringUtil::Length(importedMaterialData.normalTexture.Ascii())) {
                material.normalTextureIndex = AddTexture(built, importedMaterialData.normalTexture);
            }

            for(uint property = 0; property < eMaterialPropertyCount; ++property) {
                material.scalarAttributeValues[property] = importedMaterialData.scalarAttributes[property];
                
                const FilePathString& textureName = importedMaterialData.scalarAttributeTextures[property];
                if(StringUtil::Length(textureName.Ascii())) {
                    material.scalarAttributeTextureIndices[property] = AddTexture(built, textureName);
                }
            }

            if(material.scalarAttributeTextureIndices[eDisplacement] != InvalidIndex32) {
                material.flags |= eDisplacementEnabled;
            }
            if(material.shader == eDisneySolid) {
                if(material.scalarAttributeValues[eDiffuseTrans] > 0.0f || material.scalarAttributeValues[eSpecTrans] > 0.0f) {
                    material.flags |= eTransparent;
                }
            }
        }

        QuickSortMatchingArrays(built->materialHashes.DataPointer(), built->materials.DataPointer(), built->materials.Count());
        return Success_;
    }

    //=============================================================================================================================
    Error BuildScene(BuildProcessorContext* context, cpointer materialPrefix, ImportedModel* imported, BuiltModel* built)
    {
        ReturnError_(ImportMaterials(context, materialPrefix, imported, built));
        BuildMeshes(imported, built);

        const float intensityScale = 1.2f;
        built->backgroundIntensity = intensityScale * float3(0.9f, 0.84f, 0.78f);
        built->cameras.Append(imported->cameras);

        return Success_;
    }
}
