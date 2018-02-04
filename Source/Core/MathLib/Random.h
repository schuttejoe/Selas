#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib\FloatStructs.h>
#include <SystemLib\BasicTypes.h>

namespace Shooty {

    namespace Random {

        uint  RandUint(uint max);
        float RandFloat0_1(void);

        void FillRandomBuffer(float* buffer, uint count, uint32 seed);
        void FillRandomBuffer(float4* buffer, uint count, uint32 seed);

        float3 UniformConeRandom(float3& direction, float thetaMax);
    };
}