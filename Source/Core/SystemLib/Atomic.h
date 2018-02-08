#pragma once

//==============================================================================
// Joe Schutte 
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Shooty {

    namespace Atomic {
        int32 Increment32(volatile int32* dest);
        int64 Increment64(volatile int64* dest);

        uint32 Increment32(volatile uint32* dest);
        uint64 Increment64(volatile uint64* dest);

        int32 Decrement32(volatile int32* dest);
        int64 Decrement64(volatile int64* dest);

        uint32 Decrement32(volatile uint32* dest);
        uint32 Decrement64(volatile uint64* dest);

        int32 Add32(volatile int32* dest, int32 add);
        int64 Add64(volatile int64* dest, int64 add);

        uint32 Add32(volatile uint32* dest, uint32 add);
        uint64 Add64(volatile uint64* dest, uint64 add);

        int32 CompareExchange32(volatile int32* dest, int32 exchange_with, int32 compare_to);
        int64 CompareExchange64(volatile int64* dest, int64 exchange_with, int64 compare_to);

    }
}