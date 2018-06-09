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
    float ElapsedMicrosecondsF(std::chrono::high_resolution_clock::time_point& since)
    {
        auto current = std::chrono::high_resolution_clock::now();

        std::chrono::duration<float, std::micro> elapsed = current - since;
        return elapsed.count();
    }

    //==============================================================================
    float SystemTime::ElapsedMillisecondsF(std::chrono::high_resolution_clock::time_point& since)
    {
        auto current = std::chrono::high_resolution_clock::now();

        std::chrono::duration<float, std::milli> elapsed = current - since;
        return elapsed.count();
    }

    //==============================================================================
    float SystemTime::ElapsedSecondsF(std::chrono::high_resolution_clock::time_point& since)
    {
        auto current = std::chrono::high_resolution_clock::now();

        std::chrono::duration<float> elapsed = current - since;
        return elapsed.count();
    }
}