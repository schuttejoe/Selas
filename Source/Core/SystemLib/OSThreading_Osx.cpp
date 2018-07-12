//==============================================================================
// Joe Schutte
//==============================================================================

#if IsOsx_

#include "SystemLib/OSThreading.h"
#include "SystemLib/MemoryAllocation.h"
#include "SystemLib/Memory.h"

#include <pthread.h>
#include <dispatch/dispatch.h>
#include <unistd.h>

namespace Selas
{
    //==============================================================================
    // Events
    //==============================================================================

    // //==============================================================================
    // void* CreateEvent(bool manualReset, bool initialState)
    // {
        
    // }

    // //==============================================================================
    // bool CloseEvent(void* handle)
    // {
        
    // }

    // //==============================================================================
    // bool SetEvent(void* handle)
    // {
        
    // }

    // //==============================================================================
    // bool ResetEvent(void* handle)
    // {
        
    // }

    // //==============================================================================
    // bool WaitForSingleObject(void* handle, uint32 milliseconds)
    // {
        
    // }

    // //==============================================================================
    // bool WaitForAllObjects(uint32 numHandles, void** handles, uint32 milliseconds)
    // {
        
    // }

    //==============================================================================
    // Semaphore
    //==============================================================================

    //==============================================================================
    void* CreateOSSemaphore(uint32 initialCount, uint32 maxCount)
    {
    	dispatch_semaphore_t semaphore = dispatch_semaphore_create(initialCount);
    	return semaphore;
    }

    //==============================================================================
    void CloseOSSemaphore(void* semaphore)
    {
        dispatch_release((dispatch_semaphore_t)semaphore);
    }

    //==============================================================================
    void PostSemaphore(void* semaphore, uint32 count)
    {
        for(uint scan = 0; scan < count; ++scan) {
        	dispatch_semaphore_signal((dispatch_semaphore_t)semaphore);
        }
    }

    //==============================================================================
    bool WaitForSemaphore(void* semaphore, uint32 milliseconds)
    {
        return (dispatch_semaphore_wait((dispatch_semaphore_t)semaphore, DISPATCH_TIME_FOREVER) == 0);
    }

    //==============================================================================
    // Spinlocks
    //==============================================================================
    void* CreateSpinLock(void)
    {
        uint8* spinlock = AllocArrayAligned_(uint8, CacheLineSize_, CacheLineSize_);
        Memory::Zero(spinlock, CacheLineSize_);

        return (void*)spinlock;
    }

    //==============================================================================
    void CreateSpinLock(uint8 spin[CacheLineSize_])
    {
        Memory::Zero(spin, CacheLineSize_);
    }

    //==============================================================================
    void CloseSpinlock(void* spinlock)
    {
        FreeAligned_(spinlock);
    }

    //==============================================================================
    bool TryEnterSpinLock(void* spinlock)
    {
        volatile int64* atom = reinterpret_cast<volatile int64*>(spinlock);
        return __sync_val_compare_and_swap(atom, 1, 0) == 1;
    }

    //==============================================================================
    void EnterSpinLock(void* spinlock)
    {
        volatile int32* address = (volatile int32*)(spinlock);
        while(__sync_val_compare_and_swap(address, 0, 1) == 1) { };
    }

    //==============================================================================
    void LeaveSpinLock(void* spinlock)
    {
     	volatile int32* address = (volatile int32*)(spinlock);
        *address = 0;
    }

    //==============================================================================
    void Sleep(uint sleepTimeMs)
    {
        usleep(sleepTimeMs * 1000);
    }
}

#endif