//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "MathLib/Sampler.h"
#include "MathLib/Trigonometric.h"
#include "SystemLib/MinMax.h"

namespace Selas
{
    //=============================================================================================================================
    void CSampler::Initialize(uint32 seed)
    {
        Random::MersenneTwisterInitialize(&twister, seed);
    }

    //=============================================================================================================================
    void CSampler::Shutdown()
    {
        Random::MersenneTwisterShutdown(&twister);
    }

    //=============================================================================================================================
    void CSampler::Reseed(uint32 seed)
    {
        Random::MersenneTwisterReseed(&twister, seed);
    }

    //=============================================================================================================================
    float CSampler::UniformFloat()
    {
        return Random::MersenneTwisterFloat(&twister);
    }

    //=============================================================================================================================
    uint32 CSampler::UniformUInt32()
    {
        return Random::MersenneTwisterUint32(&twister);
    }

    //=========================================================================================================================
    float3 CSampler::UniformSphere()
    {
        float r0 = UniformFloat();
        float r1 = UniformFloat();

        float u = 2.0f * r1 - 1.0f;
        float norm = Math::Sqrtf(Max(0.0f, 1.0f - u * u));
        float theta = Math::TwoPi_ * r0;

        return float3(norm * Math::Cosf(theta), u, norm * Math::Sinf(theta));
    }

    //=========================================================================================================================
    float CSampler::UniformSpherePdf()
    {
        return Math::Inv4Pi_;
    }
}