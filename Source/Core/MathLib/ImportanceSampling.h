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

        float IblPdf(IblDensityFunctions* distributions, float3 w);
        void Ibl(IblDensityFunctions* distributions, float r0, float r1, float& theta, float& phi, uint& x, uint& y, float& pdf);
        void ShutdownDensityFunctions(IblDensityFunctions* distributions);

        float GgxDPdf(float roughness, float dotNH);
        void GgxD(float roughness, float r0, float r1, float& theta, float& phi);
        float3 GgxVndf(float3 wo, float roughness, float u1, float u2);

        float BalanceHeuristic(uint nf, float fPdf, uint ng, float gPdf);
        float PowerHeuristic(uint nf, float fPdf, uint ng, float gPdf);
    }
}
