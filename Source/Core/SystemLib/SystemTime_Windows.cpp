//==============================================================================
// Joe Schutte
//==============================================================================

#include "SystemTime.h"

#if IsWindows_

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace Shooty {

    static_assert(sizeof(LARGE_INTEGER) == sizeof(int64), "Mismatched primitive size");

    void SystemTime::GetCycleFrequency(int64* frequency) {
        QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(frequency));
    }

    void SystemTime::GetCycleCounter(int64* cycles) {
        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(cycles));
    }

}

#endif