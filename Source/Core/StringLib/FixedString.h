#pragma once

#include "StringLib/StringUtil.h"
#include "IoLib/File.h"
#include "IoLib/Serializer.h"

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

namespace Selas
{
    template <int T>
    struct FixedString
    {
        FixedString() { Clear(); }

        char str[T];

        char* Ascii(void) { return &str[0]; }
        const char* Ascii(void) const { return &str[0]; }
        uint Capacity() { return T; }
        void Clear() { str[0] = '\0'; }

        void Copy(const char* copyString)
        {
            StringUtil::Copy(Ascii(), T, copyString);
        }
    };

    typedef FixedString<32>  FixedString32;
    typedef FixedString<64>  FixedString64;
    typedef FixedString<128> FixedString128;
    typedef FixedString<256> FixedString256;
    typedef FixedString<512> FixedString512;
    typedef FixedString<MaxPath_> FilePathString;

    #define FixedStringSprintf(str, msg, ...) StringUtil::Sprintf(str.Ascii(), str.Capacity(), msg, ##__VA_ARGS__)

    template <int T>
    void Serialize(CSerializer* serializer, FixedString<T>& data)
    {
        for(uint scan = 0; scan < T; ++scan) {
            Serialize(serializer, (int8&)data.str[scan]);
        }
    }
}

