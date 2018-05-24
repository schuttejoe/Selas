//==============================================================================
// Joe Schutte
//==============================================================================

#include <ThreadingLib/Thread.h>
#include <SystemLib/BasicTypes.h>
#include <Windows.h>

namespace Selas
{
    //==============================================================================
    ThreadHandle CreateThread(ThreadFunction function, void* userData)
    {
        uint32 creationFlags = 0; // or CREATE_SUSPENDED
        return ::CreateThread(/*security*/nullptr, /*stack size*/0, (LPTHREAD_START_ROUTINE)function, userData, creationFlags, /*out thread_id*/nullptr);
    }

    //==============================================================================
    void ShutdownThread(ThreadHandle threadHandle)
    {
        WaitForSingleObject((HANDLE)threadHandle, INFINITE);
        CloseHandle((HANDLE)threadHandle);
    }
}