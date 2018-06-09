//==============================================================================
// Joe Schutte
//==============================================================================

#include "SystemLib/Error.h"
#include "SystemLib/MemoryAllocation.h"

#if IsWindows_
    // -- for DebugBreak
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif IsOsx_
    #include <stdarg.h>
#endif

// -- for vsnprintf_s
#include <stdio.h>

namespace Selas
{
    //==============================================================================
    void Error::IncrementRefCount()
    {
        if(refCountPtr != nullptr) {
            ++*refCountPtr;
        }
    }

    //==============================================================================
    void Error::DecrementRefCount()
    {
        if(refCountPtr != nullptr) {
            --*refCountPtr;

            if(*refCountPtr == 0) {
                SafeFree_(errorMessage);
                SafeFree_(refCountPtr);
            }
        }
    }

    //==============================================================================
    Error::Error(const char* message, ...)
    {
        va_list varg;
        va_start(varg, message);

        #if IsWindows_
            uint32 errorStrLength = _vscprintf(message, varg) + 1;
        #elif IsOsx_
            uint32 errorStrLength = vsnprintf(NULL, 0, message, varg) + 1;
        #endif

        errorMessage = AllocArray_(char, errorStrLength);
        refCountPtr = AllocArray_(uint32, 1);
        *refCountPtr = 1;

        #if IsWindows_
            vsnprintf_s(errorMessage, errorStrLength, _TRUNCATE, message, varg);
        #elif IsOsx_
            vsnprintf(errorMessage, errorStrLength, message, varg);
        #endif

        va_end(varg);

        #if IsWindows_
            OutputDebugStringA(errorMessage);
            OutputDebugStringA("\n");
        #elif IsOsx_
            printf("%s\n", errorMessage);
        #endif

        #if IsWindows_
            DebugBreak();
        #elif IsOsx_
            __builtin_trap();
        #endif
    }

    //==============================================================================
    Error::~Error()
    {
        DecrementRefCount();
    }

    //==============================================================================
    Error::Error(const Error& error_)
    {
        errorMessage = error_.errorMessage;
        refCountPtr = error_.refCountPtr;

        IncrementRefCount();
    }

    //==============================================================================
    Error& Error::operator=(const Error& error_)
    {
        if(this == &error_) {
            return *this;
        }

        DecrementRefCount();

        errorMessage = error_.errorMessage;
        refCountPtr = error_.refCountPtr;

        IncrementRefCount();

        return *this;
    }
}