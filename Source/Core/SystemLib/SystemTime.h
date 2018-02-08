#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Shooty {

    namespace SystemTime {

        void GetCycleFrequency(int64* frequency);
        void GetCycleCounter(int64* cycles);
        float ElapsedMs(int64& prevTimestamp);

    }
}