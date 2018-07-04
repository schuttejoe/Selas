#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "MathLib/FloatStructs.h"

namespace Selas
{
    struct SurfaceDifferentials;

    struct Ray
    {
        float3 origin;
        float3 direction;
    };

    template<uint Count_>
    struct RayT
    {
        float ox[Count_];
        float oy[Count_];
        float oz[Count_];
        float dx[Count_];
        float dy[Count_];
        float dz[Count_];
    };

    typedef RayT<16> Ray16;
    typedef RayT<8>  Ray8;
    typedef RayT<4>  Ray4;

    Ray MakeRay(float3 origin, float3 direction);
}