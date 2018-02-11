#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

namespace Shooty {

    template <int T>
    struct FixedString {
        char str[T];

        char* Ascii(void) { return &str[0]; }
        uint Capcaity() { return T; }
    };

    typedef FixedString<32>  FixedString32;
    typedef FixedString<64>  FixedString64;
    typedef FixedString<128> FixedString128;
    typedef FixedString<256> FixedString256;
    typedef FixedString<512> FixedString512;

}

