//==============================================================================
// Joe Schutte
//==============================================================================

#include "IoLib/FileTime.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace Selas
{
    //==============================================================================
    bool FileTime(cpointer filepath, FileTimestamp* timestamp)
    {
        HANDLE handle = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if(handle == INVALID_HANDLE_VALUE) {
            return false;
        }

        FILETIME time;
        BOOL success = GetFileTime(handle, nullptr, nullptr, &time);
        CloseHandle(handle);
        if(!success) {
            return false;
        }

        timestamp->low  = time.dwLowDateTime;
        timestamp->high = time.dwHighDateTime;

        return true;
    }

    //==============================================================================
    bool CompareFileTime(const FileTimestamp& left, const FileTimestamp& right)
    {
        FILETIME l;
        l.dwLowDateTime  = left.low;
        l.dwHighDateTime = left.high;

        FILETIME r;
        r.dwLowDateTime  = right.low;
        r.dwHighDateTime = right.high;

        return (CompareFileTime(&l, &r) == 0);
    }
}
