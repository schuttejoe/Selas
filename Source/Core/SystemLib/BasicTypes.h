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

#define ForceInline_      __forceinline
#define Unused_(x)        (void)x;
#define FloatMax_         3.402823466e+38F
#define MinFloatEpsilon_  1.192092896e-07F
#define ReturnFailure_(x) if(!x) { return false; }

namespace Shooty {

    #if Is64Bit_
        typedef uint64             uint;
        typedef int64              sint;
    #else
        typedef uint32             uint;
        typedef int32              sint;
    #endif

}
