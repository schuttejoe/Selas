#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

namespace Shooty {

    template<typename T>
    inline T Min(T const& lhs, T const& rhs) {
        return (lhs < rhs) ? lhs : rhs;
    }

    template<typename T>
    inline T Max(T const& lhs, T const& rhs) {
        return (lhs > rhs) ? lhs : rhs;
    }

}