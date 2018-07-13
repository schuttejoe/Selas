//==============================================================================
// Joe Schutte
//==============================================================================

#include "MathLib/Sampler.h"

namespace Selas
{
    //==============================================================================
    void CSampler::Initialize(uint32 seed)
    {
        Random::MersenneTwisterInitialize(&twister, seed);
    }

    //==============================================================================
    void CSampler::Shutdown()
    {
        Random::MersenneTwisterShutdown(&twister);
    }

    //==============================================================================
    void CSampler::Reseed(uint32 seed)
    {
        Random::MersenneTwisterReseed(&twister, seed);
    }

    //==============================================================================
    float CSampler::UniformFloat()
    {
        return Random::MersenneTwisterFloat(&twister);
    }

    //==============================================================================
    uint32 CSampler::UniformUInt32()
    {
        return Random::MersenneTwisterUint32(&twister);
    }
}