//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/ImportModel.h"
#include "BuildCore/BuildContext.h"
#include "GeometryLib/CoordinateSystem.h"
#include "MathLib/FloatFuncs.h"
#include "StringLib/FixedString.h"
#include "SystemLib/MemoryAllocation.h"
#include "SystemLib/JsAssert.h"
#include "SystemLib/Logging.h"

// -- middleware
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <stdio.h>

namespace Selas
{
    #define AssImpVec3ToFloat3_(v) float3(v.x, v.y, v.z)

    //=============================================================================================================================
    static Error CountMeshes(const aiScene* aiscene, const aiNode* node, uint& count)
    {
        count += node->mNumMeshes;

        for(uint scan = 0; scan < node->mNumChildren; ++scan) {
            ReturnError_(CountMeshes(aiscene, node->mChildren[scan], count));
        }

        return Success_;
    }

    //=============================================================================================================================
    static void ExtractTriangles(const aiFace* face, CArray<uint32>& indices)
    {
        Assert_(face->mNumIndices == 3);
        indices.Add(face->mIndices[0]);
        indices.Add(face->mIndices[1]);
        indices.Add(face->mIndices[2]);
    }

    //=============================================================================================================================
    static void ExtractQuad(const aiFace* face, CArray<uint32>& indices)
    {
        Assert_(face->mNumIndices == 4);
        indices.Add(face->mIndices[0]);
        indices.Add(face->mIndices[1]);
        indices.Add(face->mIndices[2]);
        indices.Add(face->mIndices[3]);
    }

    //=============================================================================================================================
    static void ExtractMaterials(const aiScene* aiscene, ImportedModel* model)
    {
        uint32 materialCount = aiscene->mNumMaterials;

        model->materials.Resize(materialCount);
        model->materialHashes.Resize(materialCount);
        for(uint scan = 0; scan < materialCount; ++scan) {
            aiMaterial* material = aiscene->mMaterials[scan];
            aiString name;
            material->Get(AI_MATKEY_NAME, name);

            model->materials[scan].Copy(name.data);
            model->materialHashes[scan] = MurmurHash3_x86_32(name.data, StringUtil::Length(name.data), 0);
        }
    }

    //=============================================================================================================================
    static Error ExtractMeshes(const aiScene* aiscene, const aiNode* node, ImportedModel* model, uint& meshIndex)
    {
        for(uint meshscan = 0, meshcount = node->mNumMeshes; meshscan < meshcount; ++meshscan) {
            const struct aiMesh* aimesh = aiscene->mMeshes[node->mMeshes[meshscan]];

            ImportedMesh* mesh = model->meshes[meshIndex++];

            bool hasNans = false;

            mesh->materialHash = model->materialHashes[aimesh->mMaterialIndex];
            mesh->nameHash = MurmurHash3_x86_32(aimesh->mName.C_Str(), StringUtil::Length(aimesh->mName.C_Str()), 0);
            mesh->name.Copy(aimesh->mName.C_Str());

            // -- extract vertices
            uint vertexcount = aimesh->mNumVertices;
            mesh->positions.Resize((uint32)vertexcount);
            for(uint scan = 0; scan < vertexcount; ++scan) {
                mesh->positions[scan].x = aimesh->mVertices[scan].x;
                mesh->positions[scan].y = aimesh->mVertices[scan].y;
                mesh->positions[scan].z = aimesh->mVertices[scan].z;
            }

            // -- extract normals
            if(aimesh->HasNormals()) {
                mesh->normals.Resize((uint32)vertexcount);
                for(uint scan = 0; scan < vertexcount; ++scan) {
                    mesh->normals[scan].x = aimesh->mNormals[scan].x;
                    mesh->normals[scan].y = aimesh->mNormals[scan].y;
                    mesh->normals[scan].z = aimesh->mNormals[scan].z;
                    if(Math::IsNaN(mesh->normals[scan].x) || Math::IsNaN(mesh->normals[scan].y) 
                        || Math::IsNaN(mesh->normals[scan].z)) {
                        hasNans = true;
                        mesh->normals[scan] = float3::YAxis_;
                    }
                    
                    Assert_(Length(mesh->normals[scan]) > 0.0f);
                }
            }

            // -- extract uvs
            if(aimesh->HasTextureCoords(0)) {
                mesh->uv0.Resize((uint32)vertexcount);
                for(uint scan = 0; scan < vertexcount; ++scan) {
                    mesh->uv0[scan].x = aimesh->mTextureCoords[0][scan].x;
                    mesh->uv0[scan].y = aimesh->mTextureCoords[0][scan].y;
                }
            }

            if(aimesh->HasTangentsAndBitangents()) {
                mesh->tangents.Resize((uint32)vertexcount);
                mesh->bitangents.Resize((uint32)vertexcount);
                for(uint scan = 0; scan < vertexcount; ++scan) {
                    mesh->tangents[scan] = AssImpVec3ToFloat3_(aimesh->mTangents[scan]);
                    mesh->bitangents[scan] = AssImpVec3ToFloat3_(aimesh->mBitangents[scan]);

                    bool hackCoordinateFrame = false;
                    if(Math::IsNaN(mesh->tangents[scan].x) || Math::IsNaN(mesh->tangents[scan].y)
                        || Math::IsNaN(mesh->tangents[scan].z)) {
                        hasNans = true;
                        hackCoordinateFrame = true;
                    }
                    if(Math::IsNaN(mesh->bitangents[scan].x) || Math::IsNaN(mesh->bitangents[scan].y) 
                        || Math::IsNaN(mesh->bitangents[scan].z)) {
                        hasNans = true;
                        hackCoordinateFrame = true;
                    }

                    if(hackCoordinateFrame) {
                        MakeOrthogonalCoordinateSystem(mesh->normals[scan], &mesh->tangents[scan], &mesh->bitangents[scan]);
                    }
                }
            }

            // -- count the amount of space we'll need for triangle and quad indices
            uint32 triindexcount = 0;
            uint32 quadindexcount = 0;
            for(uint facescan = 0, facecount = aimesh->mNumFaces; facescan < facecount; ++facescan) {
                const struct aiFace* face = &aimesh->mFaces[facescan];
                if(face->mNumIndices == 3) {
                    triindexcount += 3;
                }
                else if(face->mNumIndices == 4) {
                    quadindexcount += 4;
                }
            }

            // -- extract triangle and quad indices
            mesh->triindices.Reserve(triindexcount);
            mesh->quadindices.Reserve(quadindexcount);
            for(uint facescan = 0, facecount = aimesh->mNumFaces; facescan < facecount; ++facescan) {
                const struct aiFace* face = &aimesh->mFaces[facescan];

                if(face->mNumIndices == 3) {
                    ExtractTriangles(face, mesh->triindices);
                }
                else if(face->mNumIndices == 4) {
                    ExtractQuad(face, mesh->quadindices);
                }
                else {
                    return Error_("Unsupported index count %d", face->mNumIndices);
                }
            }

            if(hasNans) {
                WriteDebugInfo_("Found NaNs in tangent space of mesh '%s'. Replacing with random coordinate frame.",
                                aimesh->mName.C_Str());
            }
        }

        for(uint scan = 0; scan < node->mNumChildren; ++scan) {
            ReturnError_(ExtractMeshes(aiscene, node->mChildren[scan], model, meshIndex));
        }

        return Success_;
    }

    //=============================================================================================================================
    static Error ExtractCamera(const aiScene* aiscene, ImportedModel* model)
    {
        model->cameras.Resize(aiscene->mNumCameras);
        for(uint scan = 0; scan < aiscene->mNumCameras; ++scan)
        {
            model->cameras[scan].position = AssImpVec3ToFloat3_(aiscene->mCameras[scan]->mPosition);
            model->cameras[scan].lookAt   = model->cameras[scan].position + AssImpVec3ToFloat3_(aiscene->mCameras[scan]->mLookAt);
            model->cameras[scan].up       = AssImpVec3ToFloat3_(aiscene->mCameras[scan]->mUp);
            model->cameras[scan].fovDegrees = aiscene->mCameras[scan]->mHorizontalFOV * Math::RadiansToDegrees_;
            model->cameras[scan].znear    = aiscene->mCameras[scan]->mClipPlaneNear;
            model->cameras[scan].zfar     = aiscene->mCameras[scan]->mClipPlaneFar;
        }

        return Success_;
    }

    //=============================================================================================================================
    Error ImportModel(BuildProcessorContext* context, ImportedModel* model)
    {
        FilePathString filepath;
        AssetFileUtils::ContentFilePath(context->source.name.Ascii(), filepath);
        context->AddFileDependency(filepath.Ascii());

        Assimp::Importer importer;
        const aiScene* aiscene = importer.ReadFile(filepath.Ascii(), 0);
        if(!aiscene) {
            const char* errstr = importer.GetErrorString();
            return Error_("AssetImporter failed with error: %s", errstr);
        }

        uint meshcount = 0;
        ReturnError_(CountMeshes(aiscene, aiscene->mRootNode, meshcount));

        model->meshes.Resize((uint32)meshcount);
        for(uint scan = 0; scan < meshcount; ++scan) {
            model->meshes[scan] = New_(ImportedMesh);
        }

        ExtractMaterials(aiscene, model);

        uint usedMeshCount = 0;
        ReturnError_(ExtractMeshes(aiscene, aiscene->mRootNode, model, usedMeshCount));
        ReturnError_(ExtractCamera(aiscene, model));

        // aiscene is cleaned up by the importer's destructor
        return Success_;
    }

    //=============================================================================================================================
    void ShutdownImportedModel(ImportedModel* model)
    {
        for(uint scan = 0, meshcount = model->meshes.Count(); scan < meshcount; ++scan) {
            Delete_(model->meshes[scan]);
        }

        model->meshes.Shutdown();
    }
}