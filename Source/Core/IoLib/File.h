#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Shooty {

    //==============================================================================
    namespace File {
        bool ReadWholeFile(const char* filename, void** fileData, uint32* fileSize);
        bool WriteWholeFile(const char* filename, void* data, uint32 size);
    };

}