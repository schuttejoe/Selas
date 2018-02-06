//==============================================================================
// Joe Schutte
//==============================================================================

#include <SceneLib/SceneResource.h>
#include <IoLib/BinarySerializer.h>
#include <IoLib/File.h>
#include <SystemLib/BasicTypes.h>

#define ReturnFailure_(code) if (!code) { return false; }

namespace Shooty {

    //==============================================================================
    bool ReadSceneResource(cpointer filepath, SceneResource* data) {

        void* fileData = nullptr;
        uint32 fileSize = 0;
        ReturnFailure_(File::ReadWholeFile(filepath, &fileData, &fileSize));

        BinaryReader reader;
        SerializerStart(&reader, fileData, fileSize);

        SerializerAttach(&reader, reinterpret_cast<void**>(&data->data), fileSize);
        FixupPointerX64(fileData, data->data->meshDatas);
        FixupPointerX64(fileData, data->data->indices);
        FixupPointerX64(fileData, data->data->positions);
        FixupPointerX64(fileData, data->data->normals);
        FixupPointerX64(fileData, data->data->uv0);       

        SerializerEnd(&reader);

        return true;
    }

}