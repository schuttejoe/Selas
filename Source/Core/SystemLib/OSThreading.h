#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "SystemLib/BasicTypes.h"

namespace Selas
{
    #define CacheLineSize_ 64

    // Events
    // -- These are not well supported by linux and they are unused for this project so disabling them for now.
    // void*    CreateEvent(bool manual_reset, bool initialState);
    // bool     CloseEvent(void* handle);
    // bool     SetEvent(void* handle);
    // bool     ResetEvent(void* handle);
    // bool     WaitForSingleObject(void* handle, uint32 milliseconds);
    // bool     WaitForAllObjects(uint32 handleCount, void** handles, uint32 milliseconds);

    // Semaphores
    void*    CreateOSSemaphore(uint32 initialCount, uint32 maxCount);
    void     CloseOSSemaphore(void* semaphore);
    void     PostSemaphore(void* semaphore, uint32 count);
    bool     WaitForSemaphore(void* semaphore, uint32 milliseconds);

    // Spinlocks
    void*    CreateSpinLock(void);
    void     CreateSpinLock(uint8 spin[CacheLineSize_]);
    void     CloseSpinlock(void* spinlock);
    bool     TryEnterSpinLock(void* spinlock);
    void     EnterSpinLock(void* spinlock);
    void     LeaveSpinLock(void* spinlock);

    // Sleep
    void     Sleep(uint sleepTimeMs);
}