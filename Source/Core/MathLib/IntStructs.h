#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    struct int2
    {
        int32 x, y;
        ForceInline_ int2() {}
        ForceInline_ int2(int32 a, int32 b) : x(a), y(b) {}
    };

    struct int3
    {
        int32 x, y, z;
        ForceInline_ int3() {}
        ForceInline_ int3(int32 a, int32 b, int32 c) : x(a), y(b), z(c) {}
    };

    struct int4
    {
        int32 x, y, z, w;
        ForceInline_ int4() {}
        ForceInline_ int4(int32 a, int32 b, int32 c, int32 d) : x(a), y(b), z(c), w(d) {}
    };
}