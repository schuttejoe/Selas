//==============================================================================
// Joe Schutte
//==============================================================================

#include "MathLib/Random.h"
#include "MathLib/FloatStructs.h"
#include "MathLib/FloatFuncs.h"
#include "MathLib/Quaternion.h"
#include "MathLib/Trigonometric.h"
#include "SystemLib/MemoryAllocation.h"
#include "SystemLib/JsAssert.h"

#include <stdlib.h>

#include <random>

namespace Selas
{
    namespace Random
    {
        //==============================================================================
        struct MersenneTwisterData
        {
            std::mt19937 mtEngine;
        };

        //==============================================================================
        void MersenneTwisterInitialize(MersenneTwister* twister, uint32 seed)
        {
            twister->data = New_(MersenneTwisterData);
            twister->data->mtEngine.seed(seed);
        }

        //==============================================================================
        void MersenneTwisterReseed(MersenneTwister* twister, uint32 seed)
        {
            twister->data->mtEngine.seed(seed);
        }

        //==============================================================================
        void MersenneTwisterShutdown(MersenneTwister* twister)
        {
            SafeDelete_(twister->data);
        }

        //==============================================================================
        float MersenneTwisterFloat(MersenneTwister* twister)
        {
            return twister->data->mtEngine() / (float)(0xFFFFFFFF);
        }

        uint32 MersenneTwisterUint32(MersenneTwister* twister)
        {
            return twister->data->mtEngine();
        }

        //==============================================================================
        uint RandUint(uint max)
        {
            return rand() % max;
        }

        //==============================================================================
        float RandFloat0_1(void)
        {
            float random = (float)rand() / (float)RAND_MAX;
            Assert_(random >= 0.0f && random <= 1.0f);
            return random;
        }
    }
}