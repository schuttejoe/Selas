#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/JsAssert.h>
#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    template <typename To_, typename From_>
    ForceInline_ To_ CheckedCast(From_ from)
    {
        Assert_(from < (int64(1) << int64(sizeof(To_) * 8)));
        return static_cast<To_>(from);
    }
}
