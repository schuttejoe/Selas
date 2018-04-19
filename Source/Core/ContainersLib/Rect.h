#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    struct Rect
    {
        sint left;
        sint top;
        sint right;
        sint bottom;
    };

    struct FloatRect
    {
        float left;
        float top;
        float right;
        float bottom;
    };
}