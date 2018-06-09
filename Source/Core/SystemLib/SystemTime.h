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
}