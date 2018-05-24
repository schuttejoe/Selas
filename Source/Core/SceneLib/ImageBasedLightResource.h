#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/ImportanceSampling.h>
#include <MathLib/FloatStructs.h>
#include <SystemLib/BasicTypes.h>

namespace Selas
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

    //==============================================================================
    // -- reading image based light resource data from disk
    bool ReadImageBasedLightResource(cpointer filepath, ImageBasedLightResource* resource);

    //==============================================================================
    // -- functions used in build to set up the conditional and marginal density functions
    uint CalculateMarginalDensityFunctionCount(uint width, uint height);
    uint CalculateConditionalDensityFunctionsCount(uint width, uint height);

    //==============================================================================
    // -- Importance sampling functions
    void Ibl(const IblDensityFunctions* distributions, float r0, float r1, float& theta, float& phi, uint& x, uint& y, float& pdf);

    //==============================================================================
    // -- Sampling the ibl directly
    float3 SampleIbl(const ImageBasedLightResourceData* ibl, float3 wi, float& pdf);
    float3 SampleIbl(const ImageBasedLightResourceData* ibl, uint x, uint y);

    //==============================================================================
    // -- cleanup
    void ShutdownDensityFunctions(IblDensityFunctions* distributions);
    
}