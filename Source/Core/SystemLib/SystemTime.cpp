//==============================================================================
// Joe Schutte
//==============================================================================

#include "SystemLib/SystemTime.h"

#include <chrono>
#include <ctime>

namespace Selas
{
    //==============================================================================
    std::chrono::high_resolution_clock::time_point SystemTime::Now()
    {
        return std::chrono::high_resolution_clock::now();
    }

    //==============================================================================
    float SystemTime::ElapsedMillisecondsF(std::chrono::high_resolution_clock::time_point& since)
    {
        auto current = std::chrono::high_resolution_clock::now();

        return std::chrono::duration_cast<std::chrono::milliseconds>(current - since).count();
    }

    //==============================================================================
    float SystemTime::ElapsedSecondsF(std::chrono::high_resolution_clock::time_point& since)
    {
        auto current = std::chrono::high_resolution_clock::now();

        return std::chrono::duration_cast<std::chrono::seconds>(current - since).count();
    }
}