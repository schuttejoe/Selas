#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/ImportanceSampling.h>
#include <MathLib/FloatStructs.h>
#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    struct IblDensityFunctions
    {
        uint64 width;
        uint64 height;
        float* marginalDensityFunction;
        float* conditionalDensityFunctions;
    };

    struct ImageBasedLightResourceData
    {
        IblDensityFunctions densityfunctions;
        float3* hdrData;
    };

    struct ImageBasedLightResource
    {
        ImageBasedLightResourceData* data;
    };

    bool ReadImageBasedLightResource(cpointer filepath, ImageBasedLightResource* resource);


    uint CalculateMarginalDensityFunctionCount(uint width, uint height);
    uint CalculateConditionalDensityFunctionsCount(uint width, uint height);

    float IblPdf(const IblDensityFunctions* distributions, float3 w);
    void Ibl(const IblDensityFunctions* distributions, float r0, float r1, float& theta, float& phi, uint& x, uint& y, float& pdf);
    void ShutdownDensityFunctions(IblDensityFunctions* distributions);

    float3 SampleIbl(const ImageBasedLightResourceData* ibl, float3 wi);
}