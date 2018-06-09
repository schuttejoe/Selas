

//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

#include "UtilityLib/MurmurHash.h"

namespace Selas
{
    //-----------------------------------------------------------------------------
    // Platform-specific functions and macros

    // Microsoft Visual Studio

    #if defined(_MSC_VER)

    #define FORCE_INLINE	__forceinline

    #include <stdlib.h>

    #define ROTL32(x,y)	_rotl(x,y)
    #define ROTL64(x,y)	_rotl64(x,y)

    #define BIG_CONSTANT(x) (x)

    // Other compilers

    #else	// defined(_MSC_VER)

    #define	FORCE_INLINE inline __attribute__((always_inline))

    inline uint32 rotl32(uint32 x, int8_t r)
    {
        return (x << r) | (x >> (32 - r));
    }

    inline uint64 rotl64(uint64 x, int8_t r)
    {
        return (x << r) | (x >> (64 - r));
    }

    #define	ROTL32(x,y)	rotl32(x,y)
    #define ROTL64(x,y)	rotl64(x,y)

    #define BIG_CONSTANT(x) (x##LLU)

    #endif // !defined(_MSC_VER)

    //-----------------------------------------------------------------------------
    // Block read - if your platform needs to do endian-swapping or can only
    // handle aligned reads, do the conversion here

    FORCE_INLINE uint32 getblock32(const uint32 * p, int i)
    {
        return p[i];
    }

    FORCE_INLINE uint64 getblock64(const uint64 * p, int i)
    {
        return p[i];
    }

    //-----------------------------------------------------------------------------
    // Finalization mix - force all bits of a hash block to avalanche

    FORCE_INLINE uint32 fmix32(uint32 h)
    {
        h ^= h >> 16;
        h *= 0x85ebca6b;
        h ^= h >> 13;
        h *= 0xc2b2ae35;
        h ^= h >> 16;

        return h;
    }

    //----------

    FORCE_INLINE uint64 fmix64(uint64 k)
    {
        k ^= k >> 33;
        k *= BIG_CONSTANT(0xff51afd7ed558ccd);
        k ^= k >> 33;
        k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
        k ^= k >> 33;

        return k;
    }

    //-----------------------------------------------------------------------------

    Hash32 MurmurHash3_x86_32(const void* key, int32 len, uint32 seed)
    {
        const uint8 * data = (const uint8*)key;
        const int nblocks = len / 4;

        Hash32 h1 = seed;

        const uint32 c1 = 0xcc9e2d51;
        const uint32 c2 = 0x1b873593;

        //----------
        // body

        const uint32 * blocks = (const uint32 *)(data + nblocks * 4);

        for(int i = -nblocks; i; i++) {
            uint32 k1 = getblock32(blocks, i);

            k1 *= c1;
            k1 = ROTL32(k1, 15);
            k1 *= c2;

            h1 ^= k1;
            h1 = ROTL32(h1, 13);
            h1 = h1 * 5 + 0xe6546b64;
        }

        //----------
        // tail

        const uint8 * tail = (const uint8*)(data + nblocks * 4);

        uint32 k1 = 0;

        switch(len & 3) {
        case 3: k1 ^= tail[2] << 16;
        case 2: k1 ^= tail[1] << 8;
        case 1: k1 ^= tail[0];
            k1 *= c1; k1 = ROTL32(k1, 15); k1 *= c2; h1 ^= k1;
        };

        //----------
        // finalization

        h1 ^= len;

        h1 = fmix32(h1);
        return h1;
    }

    //-----------------------------------------------------------------------------
    Hash128 MurmurHash3_x64_128(const void * key, int32 len, uint32 seed)
    {
        const uint8 * data = (const uint8*)key;
        const int nblocks = len / 16;

        uint64 h1 = seed;
        uint64 h2 = seed;

        const uint64 c1 = BIG_CONSTANT(0x87c37b91114253d5);
        const uint64 c2 = BIG_CONSTANT(0x4cf5ad432745937f);

        //----------
        // body

        const uint64 * blocks = (const uint64 *)(data);

        for(int i = 0; i < nblocks; i++) {
            uint64 k1 = getblock64(blocks, i * 2 + 0);
            uint64 k2 = getblock64(blocks, i * 2 + 1);

            k1 *= c1; k1 = ROTL64(k1, 31); k1 *= c2; h1 ^= k1;

            h1 = ROTL64(h1, 27); h1 += h2; h1 = h1 * 5 + 0x52dce729;

            k2 *= c2; k2 = ROTL64(k2, 33); k2 *= c1; h2 ^= k2;

            h2 = ROTL64(h2, 31); h2 += h1; h2 = h2 * 5 + 0x38495ab5;
        }

        //----------
        // tail

        const uint8 * tail = (const uint8*)(data + nblocks * 16);

        uint64 k1 = 0;
        uint64 k2 = 0;

        switch(len & 15) {
        case 15: k2 ^= ((uint64)tail[14]) << 48;
        case 14: k2 ^= ((uint64)tail[13]) << 40;
        case 13: k2 ^= ((uint64)tail[12]) << 32;
        case 12: k2 ^= ((uint64)tail[11]) << 24;
        case 11: k2 ^= ((uint64)tail[10]) << 16;
        case 10: k2 ^= ((uint64)tail[9]) << 8;
        case  9: k2 ^= ((uint64)tail[8]) << 0;
            k2 *= c2; k2 = ROTL64(k2, 33); k2 *= c1; h2 ^= k2;

        case  8: k1 ^= ((uint64)tail[7]) << 56;
        case  7: k1 ^= ((uint64)tail[6]) << 48;
        case  6: k1 ^= ((uint64)tail[5]) << 40;
        case  5: k1 ^= ((uint64)tail[4]) << 32;
        case  4: k1 ^= ((uint64)tail[3]) << 24;
        case  3: k1 ^= ((uint64)tail[2]) << 16;
        case  2: k1 ^= ((uint64)tail[1]) << 8;
        case  1: k1 ^= ((uint64)tail[0]) << 0;
            k1 *= c1; k1 = ROTL64(k1, 31); k1 *= c2; h1 ^= k1;
        };

        //----------
        // finalization

        h1 ^= len; h2 ^= len;

        h1 += h2;
        h2 += h1;

        h1 = fmix64(h1);
        h2 = fmix64(h2);

        h1 += h2;
        h2 += h1;

        Hash128 out;
        out.h1 = h1;
        out.h2 = h2;

        return out;
    }
}