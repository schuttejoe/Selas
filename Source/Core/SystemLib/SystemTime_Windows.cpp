//==============================================================================
// Joe Schutte
//==============================================================================

#include "SystemLib/SystemTime.h"

#if IsWindows_

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace Selas
{
    static_assert(sizeof(LARGE_INTEGER) == sizeof(int64), "Mismatched primitive size");

    //==============================================================================
    void SystemTime::GetCycleFrequency(int64* frequency)
    {
        QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(frequency));
    }

    //==============================================================================
    void SystemTime::GetCycleCounter(int64* cycles)
    {
        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(cycles));
    }

    //==============================================================================
    float SystemTime::ElapsedMs(int64& prevTimestamp)
    {
        int64 frequency;
        SystemTime::GetCycleFrequency(&frequency);

        int64 timestamp;
        SystemTime::GetCycleCounter(&timestamp);

        int64 deltatime;

        if(prevTimestamp) {
            deltatime = timestamp - prevTimestamp;
        }
        else {
            deltatime = 0;
        }

        prevTimestamp = timestamp;

        float ms = static_cast<float>(deltatime * int64(1000) * int64(100) / frequency) / 100.f;
        return ms;
    }
}

#endif