#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Selas
{

    #define CacheLineSize_ 64

    // Events
    void*    CreateEvent(bool manual_reset, bool initialState);
    bool     CloseEvent(void* handle);
    bool     SetEvent(void* handle);
    bool     ResetEvent(void* handle);
    bool     WaitForSingleObject(void* handle, uint32 milliseconds);
    bool     WaitForAllObjects(uint32 handleCount, void** handles, uint32 milliseconds);

    // Semaphores
    void*    CreateSemaphore(uint32 initialCount, uint32 maxCount);
    void     CloseSemaphore(void* semaphore);
    void     PostSemaphore(void* semaphore, uint32 count);
    bool     WaitForSemaphore(void* semaphore, uint32 milliseconds);

    // Spinlocks
    void*    CreateSpinLock(void);
    void     CreateSpinLock(uint8 spin[CacheLineSize_]);
    void     CloseSpinlock(void* spinlock);
    void     EnterSpinLock(void* spinlock);
    void     LeaveSpinLock(void* spinlock);
}