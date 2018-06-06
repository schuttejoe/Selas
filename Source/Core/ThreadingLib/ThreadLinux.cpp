//==============================================================================
// Joe Schutte
//==============================================================================

#if IsLinux_

#include <ThreadingLib/Thread.h>
#include <SystemLib/BasicTypes.h>
#include <SystemLib/MemoryAllocation.h>
#include <SystemLib/JsAssert.h>

#include <pthread.h>

namespace Selas
{
    struct ThreadData
    {
        pthread_t threadid;
        ThreadFunction function;
        void* userData;
    };

    //==============================================================================
    void* ThreadTrampoline(void* userData)
    {
        ThreadData* threadData = (ThreadData*)userData;
        threadData->function(threadData->userData);

        return nullptr;
    }

    //==============================================================================
    ThreadHandle CreateThread(ThreadFunction function, void* userData)
    {
        // JSTODO - This should return an error when it fails
        ThreadData* threadData = AllocArray_(ThreadData, 1);
        threadData->function = function;
        threadData->userData = userData;

        int32 result = pthread_create(&threadData->threadid, nullptr, ThreadTrampoline, threadData);
        Assert_(result == 0);

        return (void*)threadData;
    }

    //==============================================================================
    void ShutdownThread(ThreadHandle threadHandle)
    {
        ThreadData* threadData = (ThreadData*)threadHandle;
        int32 result = pthread_join(threadData->threadid, nullptr);
        Assert_(result == 0);

        Free_(threadData);
    }
}

#endif