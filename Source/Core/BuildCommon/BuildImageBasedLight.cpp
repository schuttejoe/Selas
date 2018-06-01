//==============================================================================
// Joe Schutte
//==============================================================================

#include <BuildCommon/BuildImageBasedLight.h>
#include <SceneLib/ImageBasedLightResource.h>
#include <TextureLib/StbImage.h>
#include <MathLib/FloatFuncs.h>
#include <SystemLib/MemoryAllocation.h>
#include <SystemLib/Memory.h>

namespace Selas
{

    //==============================================================================
    static float CieIntensity(float3 rgb)
    {
        return rgb.x * 0.2125f + rgb.y * 0.7154f + rgb.z * 0.0721f;
    }

    //==============================================================================
    static float* CalculateIntensityMap(uint width, uint height, float3* __restrict hdr)
    {

        float* intensities = AllocArray_(float, width * height);

        for(uint y = 0; y < height; ++y) {
            for(uint x = 0; x < width; ++x) {
                uint index = y * width + x;
                intensities[index] = CieIntensity(hdr[index]);
            }
        }

        return intensities;
    }

    //==============================================================================
    static void CalculateStrataDistributionFunctions(uint width, uint height, float* __restrict intensities, IblDensityFunctions* functions)
    {
        uint mdfCount = CalculateMarginalDensityFunctionCount(width, height);
        uint cdfCount = CalculateConditionalDensityFunctionsCount(width, height);

        functions->width = width;
        functions->height = height;
        functions->marginalDensityFunction = AllocArray_(float, mdfCount);
        functions->conditionalDensityFunctions = AllocArray_(float, cdfCount);

        Memory::Zero(functions->marginalDensityFunction, sizeof(float) * mdfCount);
        Memory::Zero(functions->conditionalDensityFunctions, sizeof(float) * cdfCount);

        // Calculate each of the density functions
        float marginalSum = 0.0f;
        for(uint y = 0; y < height; ++y) {

            // Sum along the row to get the normalization factor
            float conditionalSum = 0.0f;
            for(uint x = 0; x < width; ++x) {
                float intensity = intensities[y * width + x];
                conditionalSum += intensity;
            }

            // Record the max for this row in the marginal density function array
            functions->marginalDensityFunction[y] = conditionalSum;
            marginalSum += conditionalSum;

            // If the row wasn't all zeros we normalize the function and integrate the results
            if(conditionalSum > 0.0f) {
                float ooSum = 1.0f / conditionalSum;
                float integration = 0.0f;
                for(uint x = 0; x < width; ++x) {
                    uint index = y * width + x;

                    float intensity = ooSum * intensities[index];

                    integration += intensity;
                    functions->conditionalDensityFunctions[index] = integration;
                }
            }

            // -- account for floating point imprecision
            functions->conditionalDensityFunctions[y * width + width - 1] = 1.0f;
        }

        // If the entire stratum wasn't all zeros we normalize the marginal density function
        if(marginalSum > 0.0f) {
            float ooSum = 1.0f / marginalSum;
            float integration = 0.0f;
            for(uint scanY = 0; scanY < height; ++scanY) {
                integration += functions->marginalDensityFunction[scanY] * ooSum;
                functions->marginalDensityFunction[scanY] = integration;
            }
        }

        // -- account for floating point imprecision
        functions->marginalDensityFunction[height - 1] = 1.0f;
    }

    //==============================================================================
    Error ImportImageBasedLight(const char* filename, ImageBasedLightResourceData* ibl)
    {
        uint width;
        uint height;
        uint channels;
        void* raw;
        ReturnError_(StbImageRead(filename, 3, width, height, channels, raw));

        ibl->hdrData = reinterpret_cast<float3*>(raw);

        float* intensities = CalculateIntensityMap(width, height, ibl->hdrData);

        CalculateStrataDistributionFunctions(width, height, intensities, &ibl->densityfunctions);

        Free_(intensities);

        return Success_;
    }
}