#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

//#include <MathLib/FloatStructs.h>
#include <systemLib/BasicTypes.h>

namespace Shooty {

    struct float3;

    namespace ImportanceSampling {

        struct IblDensityFunctions
        {
            uint64 width;
            uint64 height;
            float* marginalDensityFunction;
            float* conditionalDensityFunctions;
        };

        uint CalculateMarginalDensityFunctionCount(uint width, uint height);
        uint CalculateConditionalDensityFunctionsCount(uint width, uint height);

        // -- Importance sample the IBL.
        // -- r0 and r1 are random numbers between 0 and 1 
        // -- theta and phi are spherical coordinates for the sample direction
        // -- x and y are row and column in the HDR texture to sample
        // -- weight is (solidAngle / pdf) to normalize your sample
        void Ibl(IblDensityFunctions* distributions, float r0, float r1,
                 float& theta, float& phi, uint& x, uint& y, float& weight);

        void ShutdownDensityFunctions(IblDensityFunctions* distributions);

        void Ggx(float roughness, float r0, float r1, float& theta, float& phi, float& weight);
    }
}
