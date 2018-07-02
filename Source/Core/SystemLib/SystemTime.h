#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "SystemLib/BasicTypes.h"

// -- JSTODO - check the headers later and make sure we're not pulling too much in
#include <ctime>
#include <chrono>

namespace Selas
{
    namespace SystemTime
    {
		std::chrono::high_resolution_clock::time_point Now();

        float ElapsedMicrosecondsF(std::chrono::high_resolution_clock::time_point& since);
        float ElapsedMillisecondsF(std::chrono::high_resolution_clock::time_point& since);
        float ElapsedSecondsF(std::chrono::high_resolution_clock::time_point& since);
    }

    // std::micro, std::milli, etc...
    template<typename Rate_>
    class CScopedTimeAccumulator
    {
    private:
        std::chrono::high_resolution_clock::time_point startTime;
        std::chrono::duration<float, Rate_>* destination;

    public:
        CScopedTimeAccumulator(std::chrono::duration<float, Rate_>* accumulator);
        ~CScopedTimeAccumulator();
    };

    //==============================================================================
    template <typename Rate_>
    CScopedTimeAccumulator<Rate_>::CScopedTimeAccumulator(std::chrono::duration<float, Rate_>* accumulator)
    {
        startTime = SystemTime::Now();
        destination = accumulator;
    }

    //==============================================================================
    template <typename Rate_>
    CScopedTimeAccumulator<Rate_>::~CScopedTimeAccumulator()
    {
        auto current = SystemTime::Now();
        std::chrono::duration<float, Rate_> elapsed = current - startTime;

        *destination += elapsed;
    }
}