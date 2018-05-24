#if IsWindows_

//==============================================================================
// Joe Schutte 
//==============================================================================

#include <SystemLib/Atomic.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>

namespace Selas
{

    //==============================================================================
    int32 Atomic::Increment32(volatile int32* destination)
    {
        static_assert(sizeof(int32) == sizeof(long), "Unexpected primitive size");

        long initial_value = InterlockedIncrement(reinterpret_cast<long volatile*>(destination));
        return initial_value;
    }

    //==============================================================================
    int64 Atomic::Increment64(volatile int64* destination)
    {
        static_assert(sizeof(int64) == sizeof(long long), "Unexpected primitive size");

        long long initial_value = InterlockedIncrement64(reinterpret_cast<long long volatile*>(destination));
        return initial_value;
    }

    //==============================================================================
    int32 Atomic::Decrement32(volatile int32* destination)
    {
        static_assert(sizeof(int32) == sizeof(long), "Unexpected primitive size");

        long initial_value = InterlockedDecrement(reinterpret_cast<long volatile*>(destination));
        return initial_value;
    }

    //==============================================================================
    int64 Atomic::Decrement64(volatile int64* destination)
    {
        static_assert(sizeof(int64) == sizeof(long long), "Unexpected primitive size");

        long long initial_value = InterlockedDecrement64(reinterpret_cast<long long volatile*>(destination));
        return initial_value;
    }

    //==============================================================================
    int32 Atomic::Add32(volatile int32* destination, int32 addValue)
    {
        static_assert(sizeof(int32) == sizeof(long), "Unexpected primitive size");

        long initial_value = InterlockedExchangeAdd(reinterpret_cast<long volatile*>(destination), addValue);
        return initial_value;
    }

    //==============================================================================
    int64 Atomic::Add64(volatile int64* destination, int64 addValue)
    {
        static_assert(sizeof(int64) == sizeof(long long), "Unexpected primitive size");

        long long initial_value = InterlockedExchangeAdd64(reinterpret_cast<long long volatile*>(destination), addValue);
        return initial_value;
    }

    //==============================================================================
    uint32 Atomic::Add32(volatile uint32* destination, uint32 addValue)
    {
        static_assert(sizeof(int32) == sizeof(long), "Unexpected primitive size");

        long initial_value = InterlockedExchangeAdd(reinterpret_cast<long volatile*>(destination), static_cast<int32>(addValue));
        return initial_value;
    }

    //==============================================================================
    uint64 Atomic::Add64(volatile uint64* destAddend, uint64 addValue)
    {
        static_assert(sizeof(uint64) == sizeof(long long), "Unexpected primitive size");

        long long initial_value = InterlockedExchangeAdd64(reinterpret_cast<long long volatile*>(destAddend), static_cast<int64>(addValue));
        return initial_value;
    }

    //==============================================================================
    int32 Atomic::CompareExchange32(volatile int32* destination, int32 exchangeWith, int32 compareTo)
    {
        static_assert(sizeof(int32) == sizeof(long), "Unexpected primitive size");

        long initial_value = InterlockedCompareExchange(reinterpret_cast<long volatile*>(destination), exchangeWith, compareTo);
        return initial_value;
    }

    //==============================================================================
    int64 Atomic::CompareExchange64(volatile int64* destination, int64 exchangeWith, int64 compareTo)
    {
        static_assert(sizeof(int64) == sizeof(long long), "Unexpected primitive size");

        long long initial_value = InterlockedCompareExchange64(reinterpret_cast<long long volatile*>(destination), exchangeWith, compareTo);
        return initial_value;
    }
}

#endif