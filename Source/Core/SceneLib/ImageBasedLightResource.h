#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "MathLib/ImportanceSampling.h"
#include "MathLib/FloatStructs.h"
#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    class CSerializer;

    struct IblDensityFunctions
    {
        uint64 width;
        uint64 height;
        float* marginalDensityFunction;
        float* conditionalDensityFunctions;
    };

    struct ImageBasedLightResourceData
    {
        ImageBasedLightResourceData() : pad(0) {}

        IblDensityFunctions densityfunctions;
        uint64 missWidth;
        uint64 missHeight;
        float rotationRadians;
        uint32 pad;

        float3* lightData;
        float3* missData;
    };
    void Serialize(CSerializer* serializer, ImageBasedLightResourceData& data);

    struct ImageBasedLightResource
    {
        static cpointer kDataType;
        static const uint64 kDataVersion;

        ImageBasedLightResourceData* data;

        ImageBasedLightResource();
        ~ImageBasedLightResource();
    };

    //=============================================================================================================================
    // -- reading image based light resource data from disk
    Error ReadImageBasedLightResource(cpointer assetname, ImageBasedLightResource* resource);
    void ShutdownImageBasedLightResource(ImageBasedLightResource* resource);

    //=============================================================================================================================
    // -- functions used in build to set up the conditional and marginal density functions
    uint CalculateMarginalDensityFunctionCount(uint width, uint height);
    uint CalculateConditionalDensityFunctionsCount(uint width, uint height);

    //=============================================================================================================================
    // -- Importance sampling functions
    void Ibl(const ImageBasedLightResourceData* ibl, float r0, float r1, float& theta, float& phi, uint& x, uint& y, float& pdf);

    //=============================================================================================================================
    // -- Sampling the ibl directly
    float3 SampleIbl(const ImageBasedLightResourceData* ibl, float3 wi, float& pdf);
    float3 SampleIblMiss(const ImageBasedLightResourceData* ibl, float3 wi, float& pdf);
    float SampleIBlPdf(const ImageBasedLightResourceData* ibl, float3 wi);
    float3 SampleIbl(const ImageBasedLightResourceData* ibl, uint x, uint y);

    //=============================================================================================================================
    // -- cleanup
    void ShutdownDensityFunctions(IblDensityFunctions* distributions);
    
}