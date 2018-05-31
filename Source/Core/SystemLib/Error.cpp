//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/Error.h>
#include <SystemLib/MemoryAllocation.h>

// -- for DebugBreak
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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

        uint32 errorStrLength = _vscprintf(message, varg) + 1;

        errorMessage = AllocArray_(char, errorStrLength);
        refCountPtr = AllocArray_(uint32, 1);
        *refCountPtr = 1;

        vsnprintf_s(errorMessage, errorStrLength, _TRUNCATE, message, varg);

        va_end(varg);

        OutputDebugStringA(errorMessage);
        OutputDebugStringA("\n");

        DebugBreak();
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