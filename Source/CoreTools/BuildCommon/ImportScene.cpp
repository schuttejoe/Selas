//==============================================================================
// Joe Schutte
//==============================================================================

#include <BuildCommon/ImportScene.h>
#include <MathLib/FloatFuncs.h>
#include <SystemLib/MemoryAllocation.h>

// -- middleware
#include "assimp/Importer.hpp"
#include "assimp/Scene.h"
#include "assimp/postprocess.h"

namespace Shooty {

    #define ReturnFailure_(x) if(!x) { return false; }
    #define AssImpVec3ToFloat3_(v) float3(v.x, v.y, v.z)

    //==============================================================================
    static bool CountMeshes(const aiScene* aiscene, const aiNode* node, uint& count) {
        count += node->mNumMeshes;

        for (uint scan = 0; scan < node->mNumChildren; ++scan) {
            ReturnFailure_(CountMeshes(aiscene, node->mChildren[scan], count));
        }

        return true;
    }

    //==============================================================================
    static void ExtractTriangles(const aiFace* face, CArray<uint32>& indices) {
        Assert_(face->mNumIndices == 3);
        indices.Add(face->mIndices[0]);
        indices.Add(face->mIndices[1]);
        indices.Add(face->mIndices[2]);
    }

    //==============================================================================
    static void ExtractQuad(const aiFace* face, CArray<uint32>& indices) {
        Assert_(face->mNumIndices == 4);

        Error_("Not tested");
        indices.Add(face->mIndices[0]);
        indices.Add(face->mIndices[1]);
        indices.Add(face->mIndices[2]);
        indices.Add(face->mIndices[0]);
        indices.Add(face->mIndices[2]);
        indices.Add(face->mIndices[3]);
    }

    //==============================================================================
    static bool ExtractMeshes(const aiScene* aiscene, const aiNode* node, ImportedScene* scene, uint& meshIndex) {
        for (uint meshscan = 0, meshcount = node->mNumMeshes; meshscan < meshcount; ++meshscan) {
            const struct aiMesh* aimesh = aiscene->mMeshes[node->mMeshes[meshscan]];

            ImportedMesh* mesh = scene->meshes[meshIndex++];

            // -- extract vertices
            uint vertexcount = aimesh->mNumVertices;
            mesh->positions.Resize((uint32)vertexcount);
            for (uint scan = 0; scan < vertexcount; ++scan) {
                mesh->positions[scan].x = aimesh->mVertices[scan].x;
                mesh->positions[scan].y = aimesh->mVertices[scan].y;
                mesh->positions[scan].z = aimesh->mVertices[scan].z;
            }

            // -- extract normals
            if (aimesh->HasNormals()) {
                mesh->normals.Resize((uint32)vertexcount);
                for (uint scan = 0; scan < vertexcount; ++scan) {
                    mesh->normals[scan].x = aimesh->mNormals[scan].x;
                    mesh->normals[scan].y = aimesh->mNormals[scan].y;
                    mesh->normals[scan].z = aimesh->mNormals[scan].z;
                }
            }

            // -- extract uvs
            if (aimesh->HasTextureCoords(0)) {
                mesh->uv0.Resize((uint32)vertexcount);
                for (uint scan = 0; scan < vertexcount; ++scan) {
                    mesh->uv0[scan].x = aimesh->mTextureCoords[0][scan].x;
                    mesh->uv0[scan].y = aimesh->mTextureCoords[0][scan].y;
                }
            }

            // -- extract indices
            mesh->indices.Reserve((uint32)aimesh->mNumFaces * 3);
            for (uint facescan = 0, facecount = aimesh->mNumFaces; facescan < facecount; ++facescan) {
                const struct aiFace* face = &aimesh->mFaces[facescan];

                if (face->mNumIndices == 3) {
                    ExtractTriangles(face, mesh->indices);
                }
                else if (face->mNumIndices == 4) {
                    ExtractQuad(face, mesh->indices);
                }
                else {
                    return false;
                }
            }
        }

        for (uint scan = 0; scan < node->mNumChildren; ++scan) {
            ReturnFailure_(ExtractMeshes(aiscene, node->mChildren[scan], scene, meshIndex));
        }

        return true;
    }

    //==============================================================================
    static bool ExtractCamera(const aiScene* aiscene, ImportedScene* scene)
    {
        // -- prepare defaults
        scene->camera.position = float3(0.0f, 0.0f, 5.0f);
        scene->camera.lookAt = float3(0.0f, 0.0f, 0.0f);
        scene->camera.up = float3(0.0f, 1.0f, 0.0f);
        scene->camera.fov = 45.0f * Math::DegreesToRadians_;
        scene->camera.znear = 0.1f;
        scene->camera.zfar = 500.0f;

        // -- just grabbing the first camera for now
        if(aiscene->mNumCameras > 0) {

            scene->camera.position = AssImpVec3ToFloat3_(aiscene->mCameras[0]->mPosition);
            scene->camera.lookAt = AssImpVec3ToFloat3_(aiscene->mCameras[0]->mLookAt);
            scene->camera.up = AssImpVec3ToFloat3_(aiscene->mCameras[0]->mUp);
            scene->camera.fov = 2.0f * aiscene->mCameras[0]->mHorizontalFOV;
            scene->camera.znear = aiscene->mCameras[0]->mClipPlaneNear;
            scene->camera.zfar = aiscene->mCameras[0]->mClipPlaneFar;
        }

        return true;
    }

    //==============================================================================
    bool ImportScene(const char* filename, ImportedScene* scene) {
        Assimp::Importer importer;
        const aiScene* aiscene = importer.ReadFile(filename, aiProcess_GenNormals
                                                   | aiProcess_CalcTangentSpace
                                                   | aiProcess_GenUVCoords
                                                   | aiProcess_ConvertToLeftHanded
                                                   | aiProcess_Triangulate
                                                   | aiProcess_JoinIdenticalVertices
                                                   | aiProcess_PreTransformVertices
                                                   | aiProcess_TransformUVCoords
                                                   | aiProcess_ImproveCacheLocality
                                                   | aiProcess_OptimizeMeshes
                                                   | aiProcess_OptimizeGraph);
        if (!aiscene) {
            const char* errstr = importer.GetErrorString();
            (void)errstr; // JSTODO - Error reporting
            return false;
        }

        uint meshcount = 0;
        ReturnFailure_(CountMeshes(aiscene, aiscene->mRootNode, meshcount));

        scene->meshes.Resize((uint32)meshcount);
        for (uint scan = 0; scan < meshcount; ++scan) {
            scene->meshes[scan] = New_(ImportedMesh);
        }

        uint mesh_index = 0;
        ReturnFailure_(ExtractMeshes(aiscene, aiscene->mRootNode, scene, mesh_index));
        ReturnFailure_(ExtractCamera(aiscene, scene));

        // aiscene is cleaned up by the importer's destructor
        return true;
    }

    //==============================================================================
    void ShutdownImportedScene(ImportedScene* scene) {
        for (uint scan = 0, meshcount = scene->meshes.Length(); scan < meshcount; ++scan) {
            Delete_(scene->meshes[scan]);
        }

        scene->meshes.Close();
    }
}