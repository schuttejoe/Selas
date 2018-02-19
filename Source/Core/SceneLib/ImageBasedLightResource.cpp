//==============================================================================
// Joe Schutte
//==============================================================================

#include <SceneLib/ImageBasedLightResource.h>
#include <IoLib/BinarySerializer.h>
#include <IoLib/File.h>
#include <SystemLib/BasicTypes.h>

namespace Shooty {

    //==============================================================================
    bool ReadImageBasedLightResource(cpointer filepath, ImageBasedLightResource* resource) {

        void* fileData = nullptr;
        uint32 fileSize = 0;
        File::ReadWholeFile(filepath, &fileData, &fileSize);

        BinaryReader reader;
        SerializerStart(&reader, fileData, fileSize);

        SerializerAttach(&reader, reinterpret_cast<void**>(&resource->data), fileSize);
        
        FixupPointerX64(fileData, resource->data->densityfunctions.marginalDensityFunction);
        FixupPointerX64(fileData, resource->data->densityfunctions.conditionalDensityFunctions);
        FixupPointerX64(fileData, resource->data->hdrData);   

        SerializerEnd(&reader);

        return true;
    }
}