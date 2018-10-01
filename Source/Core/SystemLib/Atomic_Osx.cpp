#if IsOsx_

//=================================================================================================================================
// Joe Schutte 
//=================================================================================================================================

#include "SystemLib/Atomic.h"

#include <sys/types.h>

namespace Selas
{

    //=============================================================================================================================
    int32 Atomic::Increment32(volatile int32* destination)
    {
        int32 initialValue = __sync_fetch_and_add(destination, 1);
        return initialValue;
    }

    //=============================================================================================================================
    int64 Atomic::Increment64(volatile int64* destination)
    {
        int64 initialValue = __sync_fetch_and_add(destination, 1);
        return initialValue;
    }

    //=============================================================================================================================
    int32 Atomic::Decrement32(volatile int32* destination)
    {
        int32 initialValue = __sync_fetch_and_sub(destination, 1);
        return initialValue;
    }

    //=============================================================================================================================
    int64 Atomic::Decrement64(volatile int64* destination)
    {
        int64 initialValue = __sync_fetch_and_sub(destination, 1);
        return initialValue;
    }

    //=============================================================================================================================
    int32 Atomic::Add32(volatile int32* destination, int32 addValue)
    {
        int32 initialValue = __sync_fetch_and_add(destination, addValue);
        return initialValue;
    }

    //=============================================================================================================================
    int64 Atomic::Add64(volatile int64* destination, int64 addValue)
    {
        int64 initialValue = __sync_fetch_and_add(destination, addValue);
        return initialValue;
    }

    //=============================================================================================================================
    uint32 Atomic::AddU32(volatile uint32* destination, uint32 addValue)
    {
        uint32 initialValue = __sync_fetch_and_add(destination, addValue);
        return initialValue;
    }

    //=============================================================================================================================
    uint64 Atomic::AddU64(volatile uint64* destination, uint64 addValue)
    {
        uint64 initialValue = __sync_fetch_and_add(destination, addValue);
        return initialValue;
    }

    //=============================================================================================================================
    int32 Atomic::CompareExchange32(volatile int32* destination, int32 exchangeWith, int32 compareTo)
    {
        int32 initialValue = __sync_val_compare_and_swap(destination, compareTo, exchangeWith);
        return initialValue;
    }

    //=============================================================================================================================
    int64 Atomic::CompareExchange64(volatile int64* destination, int64 exchangeWith, int64 compareTo)
    {
        int64 initialValue = __sync_val_compare_and_swap(destination, compareTo, exchangeWith);
        return initialValue;
    }
}

#endif