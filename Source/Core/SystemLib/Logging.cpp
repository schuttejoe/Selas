//==============================================================================
// Joe Schutte
//==============================================================================

#include "SystemLib/Logging.h"

#if IsWindows_
    // -- for DebugBreak
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif IsOsx_
    #include <stdarg.h>
#endif

// -- for vsnprintf_s
#include <stdio.h>

#define MaxMessageLength_ 1024

namespace Selas
{
    //==============================================================================
    void Logging::WriteDebugInfo(const char* message, ...)
    {
        va_list varg;
        va_start(varg, message);

        char localstr[MaxMessageLength_];

        #if IsWindows_
            vsnprintf_s(localstr, MaxMessageLength_, _TRUNCATE, message, varg);
        #elif IsOsx_
            vsnprintf(localstr, MaxMessageLength_, message, varg);
        #endif

        va_end(varg);

        #if IsWindows_
            OutputDebugStringA(localstr);
            OutputDebugStringA("\n");
        #elif IsOsx_
            printf("%s\n", localstr);
        #endif
    }
}