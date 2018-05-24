#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

namespace Selas
{
    template<typename T>
    inline T Min(const T& lhs, const T& rhs)
    {
        return (lhs < rhs) ? lhs : rhs;
    }

    template<typename T>
    inline T Max(const T& lhs, const T& rhs)
    {
        return (lhs > rhs) ? lhs : rhs;
    }

    template<typename T>
    inline T Clamp(const T& value, const T& bottom, const T& top)
    {
        if(value < bottom)
            return bottom;
        if(value > top)
            return top;
        return value;
    }
}