//==============================================================================
// Joe Schutte
//==============================================================================

#if IsOsx_

#include "IoLib/FileTime.h"
#include "SystemLib/Memory.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

namespace Selas
{
    //==============================================================================
    bool FileTime(cpointer filepath, FileTimestamp* timestamp)
    {
        struct stat filestatus;

        static_assert(sizeof(timestamp->low) == sizeof(filestatus.st_mtime), "Mismatched sizes used");

        if(stat(filepath, &filestatus) == -1) {
            return false;
        }

        timestamp->low = filestatus.st_mtime;

        return true;
    }

    //==============================================================================
    bool CompareFileTime(const FileTimestamp& left, const FileTimestamp& right)
    {
        return left.low == right.low;
    }
}

#endif