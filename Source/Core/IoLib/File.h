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
        Error ReadWholeFile(cpointer filepath, void** fileData, uint32* fileSize);
        Error ReadWhileFileAsString(cpointer filepath, char** string, uint32* stringSize);
        Error WriteWholeFile(cpointer filepath, const void* data, uint32 size);

        bool Exists(cpointer filepath);
    };
}