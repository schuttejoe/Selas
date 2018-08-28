#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    #define MaxPath_ 512

    //=============================================================================================================================
    namespace File
    {
        Error ReadWholeFile(cpointer filepath, void** fileData, uint64* fileSize);
        Error ReadWhileFileAsString(cpointer filepath, char** string, uint64* stringSize);
        Error WriteWholeFile(cpointer filepath, const void* data, uint64 size);

        bool Exists(cpointer filepath);
    };
}