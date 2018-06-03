#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Selas
{
    struct FileTimestamp
    {
        unsigned long low;
        unsigned long high;
    };

    bool FileTime(cpointer filepath, FileTimestamp* timestamp);
    bool CompareFileTime(const FileTimestamp& left, const FileTimestamp& right);
}
