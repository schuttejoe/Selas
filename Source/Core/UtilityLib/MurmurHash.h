#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

// Exposes Austin Appleby's MurmurHash3 functions.

#include "SystemLib/BasicTypes.h"

namespace Selas
{
    typedef uint32 Hash32;

    struct Hash128
    {
        uint64 h1, h2;
    };

    Hash32 MurmurHash3_x86_32(const void* key, int32 len, uint32 seed);
    Hash128 MurmurHash3_x64_128(const void * key, int32 len, uint32 seed);
}