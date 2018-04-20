#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    //==============================================================================
    namespace File
    {
        bool ReadWholeFile(cpointer filepath, void** fileData, uint32* fileSize);
        bool ReadWhileFileAsString(cpointer filepath, char** string, uint32* stringSize);
        bool WriteWholeFile(cpointer filepath, void* data, uint32 size);
    };
}