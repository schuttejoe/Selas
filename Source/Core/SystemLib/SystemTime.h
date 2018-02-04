#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "BasicTypes.h"

namespace Shooty {

    namespace SystemTime {
        void GetCycleFrequency(int64* frequency);
        void GetCycleCounter(int64* cycles);

        //==============================================================================
        inline float ElapsedMs(int64& prevTimestamp) {
            int64 frequency;
            SystemTime::GetCycleFrequency(&frequency);

            int64 timestamp;
            SystemTime::GetCycleCounter(&timestamp);

            int64 deltatime;

            if (prevTimestamp) {
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
}