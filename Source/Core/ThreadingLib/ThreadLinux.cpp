//==============================================================================
// Joe Schutte
//==============================================================================

#if IsWindows_

#include <ThreadingLib/Thread.h>
#include <SystemLib/BasicTypes.h>
#include <SystemLib/MemoryAllocation.h>
#include <SystemLib/JsAssert.h>

#include <pthread.h>

namespace Selas
{
    //==============================================================================
    ThreadHandle CreateThread(ThreadFunction function, void* userData)
    {
        // JSTODO - This should return an error when it fails
        pthread_t* threadId = AllocArray_(pthread_t, 1);
        int32 result = pthread_create(threadId, nullptr, function, userData);
        Assert_(result == 0);

        return (void*)threadId;
    }

    //==============================================================================
    void ShutdownThread(ThreadHandle threadHandle)
    {
        pthread_t* threadId = (pthread_t*)threadHandle;
        int32 result = pthread_join(*threadId, nullptr);
        Assert_(result == 0);

        Free_(threadId);
    }
}

#endif