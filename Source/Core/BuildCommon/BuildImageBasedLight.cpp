//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/BuildImageBasedLight.h"
#include "BuildCore/BuildContext.h"
#include "SceneLib/ImageBasedLightResource.h"
#include "TextureLib/StbImage.h"
#include "MathLib/FloatFuncs.h"
#include "MathLib/Trigonometric.h"
#include "SystemLib/MemoryAllocation.h"
#include "SystemLib/Memory.h"

namespace Selas
{

    //=============================================================================================================================
    static float CieLuma(float3 rgb)
    {
        return rgb.x * 0.299f + rgb.y * 0.587f + rgb.z * 0.114f;
    }

    //=============================================================================================================================
    static float* CalculateIntensityMap(uint width, uint height, float3* __restrict hdr)
    {
        float* intensities = AllocArrayAligned_(float, width * height, 16);

        float heightf = (float)height;

        for(uint y = 0; y < height; ++y) {

            float theta = (y + 0.5f) * Math::Pi_ / heightf;
            float sinTheta = Math::Sinf(theta);

            for(uint x = 0; x < width; ++x) {
                uint index = y * width + x;
                intensities[index] = sinTheta * CieLuma(hdr[index]);
            }
        }

        return intensities;
    }

    //=============================================================================================================================
    static void CalculateStrataDistributionFunctions(uint width, uint height, float* __restrict intensities,
                                                     IblDensityFunctions* functions)
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

    struct Rgb16
    {
        uint16 r;
        uint16 g;
        uint16 b;
    };

    //=============================================================================================================================
    static float3 ToFloat3(Rgb16 convert)
    {
        float3 result;
        result.x = convert.r / 65535.0f;
        result.y = convert.g / 65535.0f;
        result.z = convert.b / 65535.0f;

        return result;
    }

    //=============================================================================================================================
    static Error ReadIblTextureFile(BuildProcessorContext* context, cpointer filename, uint& width, uint& height, float3*& data)
    {
        FilePathString filepath;
        AssetFileUtils::ContentFilePath(filename, filepath);

        ReturnError_(context->AddFileDependency(filepath.Ascii()));

        void* rawData;


        uint channels;
        bool floatData;
        ReturnError_(StbImageRead(filepath.Ascii(), 3, 16, width, height, channels, floatData, rawData));
        if(floatData == false) {
            data = AllocArray_(float3, width*height);
            
            Rgb16* conversion = (Rgb16*)rawData;
            for(uint scan = 0, count = width * height; scan < count; ++scan) {
                data[scan] = ToFloat3(conversion[scan]);
            }
            Free_(rawData);
        }
        else {
            data = (float3*)rawData;
        }

        return Success_;
    }

    //=============================================================================================================================
    Error ImportImageBasedLight(BuildProcessorContext* context, ImageBasedLightResourceData* ibl)
    {
        FilePathString filepath;
        AssetFileUtils::ContentFilePath(context->source.name.Ascii(), filepath);
        
        uint width;
        uint height;
        float3* raw;
        ReturnError_(ReadIblTextureFile(context, filepath.Ascii(), width, height, raw));

        ibl->lightData = raw;
        ibl->missData = nullptr;
        ibl->missWidth = 0;
        ibl->missHeight = 0;
        ibl->rotationRadians = 0.0f;
        
        float* intensities = CalculateIntensityMap(width, height, ibl->lightData);

        CalculateStrataDistributionFunctions(width, height, intensities, &ibl->densityfunctions);

        FreeAligned_(intensities);
        Free_(raw);

        return Success_;
    }

    //=============================================================================================================================
    Error ImportDualImageBasedLight(BuildProcessorContext* context, cpointer lightPath, cpointer missPath,
                                    ImageBasedLightResourceData* ibl)
    {
        uint lightWidth;
        uint lightHeight;
        float3* lightData;
        ReturnError_(ReadIblTextureFile(context, lightPath, lightWidth, lightHeight, lightData));

        uint missWidth;
        uint missHeight;
        float3* missData;
        ReturnError_(ReadIblTextureFile(context, missPath, missWidth, missHeight, missData));

        ibl->lightData = lightData;
        float* intensities = CalculateIntensityMap(lightWidth, lightHeight, ibl->lightData);
        CalculateStrataDistributionFunctions(lightWidth, lightHeight, intensities, &ibl->densityfunctions);

        ibl->missData = missData;
        ibl->missWidth = missWidth;
        ibl->missHeight = missHeight;
        ibl->rotationRadians = 0.0f;
        ibl->exposureScale = 1.0f;

        FreeAligned_(intensities);

        return Success_;
    }
}