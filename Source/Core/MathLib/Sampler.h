#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "MathLib/Random.h"
#include "MathLib/FloatStructs.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    // -- Currently just a wrapper around a Mersenne Twister but will be replaced with a better PRNG
    // -- algorithm in the not too distance future.
    class CSampler
    {
    private:
        Random::MersenneTwister twister;

    public:

        void Initialize(uint32 seed);
        void Shutdown();
        void Reseed(uint32 seed);

        float   UniformFloat();
        uint32  UniformUInt32();
    };

}