#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

typedef signed char        int8;
typedef signed short       int16;
typedef signed int         int32;
typedef signed long long   int64;

typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
typedef unsigned long long uint64;

typedef unsigned char*  pointer;
typedef const char*     cpointer;

#define ForceInline_        __forceinline
#define Unused_(x)          (void)x;
#define FloatMax_           3.402823466e+38F
#define MinFloatEpsilon_    1.192092896e-07F
#define SmallFloatEpsilon_  1e-10f
#define ReturnFailure_(x) if(!x) { return false; }
#define InvalidIndex64      uint64(-1)
#define InvalidIndex32      uint32(-1)

#define FloatInfinityBits_         0x7F800000
#define FloatNegativeInfinityBits_ 0xFF800000

namespace Shooty {

    #if Is64Bit_
        typedef uint64             uint;
        typedef int64              sint;
    #else
        typedef uint32             uint;
        typedef int32              sint;
    #endif

}
