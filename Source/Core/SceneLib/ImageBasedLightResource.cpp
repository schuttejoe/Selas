//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SceneLib/ImageBasedLightResource.h"
#include "Assets/AssetFileUtils.h"
#include "IoLib/BinaryStreamSerializer.h"
#include "IoLib/Serializer.h"
#include "IoLib/File.h"
#include "MathLib/Projection.h"
#include "MathLib/Trigonometric.h"
#include "SystemLib/BasicTypes.h"
#include "SystemLib/MinMax.h"
#include "SystemLib/JsAssert.h"

namespace Selas
{
    cpointer ImageBasedLightResource::kDataType = "IBL";
    const uint64 ImageBasedLightResource::kDataVersion = 1534822944ul;

    //=============================================================================================================================
    void Serialize(CSerializer* serializer, ImageBasedLightResourceData& data)
    {
        Serialize(serializer, data.densityfunctions.width);
        Serialize(serializer, data.densityfunctions.height);

        uint width = data.densityfunctions.width;
        uint height = data.densityfunctions.height;
        uint mdfSize = sizeof(float) * CalculateMarginalDensityFunctionCount(width, height);
        uint cdfsSize= sizeof(float) * CalculateConditionalDensityFunctionsCount(width, height);
        uint hdrDataSize = sizeof(float3) * width * height;

        serializer->SerializePtr((void*&)data.densityfunctions.marginalDensityFunction, mdfSize, 0);
        serializer->SerializePtr((void*&)data.densityfunctions.conditionalDensityFunctions, cdfsSize, 0);
        serializer->SerializePtr((void*&)data.hdrData, hdrDataSize, 0);
    }

    //=============================================================================================================================
    ImageBasedLightResource::ImageBasedLightResource()
        : data(nullptr)
    {

    }

    //=============================================================================================================================
    ImageBasedLightResource::~ImageBasedLightResource()
    {
        Assert_(data == nullptr);
    }

    //=============================================================================================================================
    Error ReadImageBasedLightResource(cpointer assetname, ImageBasedLightResource* resource)
    {
        FilePathString filepath;
        AssetFileUtils::AssetFilePath(ImageBasedLightResource::kDataType, ImageBasedLightResource::kDataVersion, assetname,
                                      filepath);

        void* fileData = nullptr;
        uint64 fileSize = 0;
        ReturnError_(File::ReadWholeFile(filepath.Ascii(), &fileData, &fileSize));

        AttachToBinary(resource->data, (uint8*)fileData, fileSize);

        return Success_;;
    }

    //=============================================================================================================================
    void ShutdownImageBasedLightResource(ImageBasedLightResource* resource)
    {
        SafeFreeAligned_(resource->data);
    }

    //=============================================================================================================================
    static void SampleDistributionFunction(float* __restrict distribution, uint count, float random01, uint& index, float& pdf)
    {
        // -- binary search the cdf to find the largest sample that is lower than the given random number between 0 and 1
        index = (uint)-1;

        sint low = 0;
        sint high = count - 1;

        while(low <= high) {
            sint mid = (low + high) / 2;

            if(distribution[mid] >= random01) {
                index = mid;
                pdf = (mid > 0) ? distribution[mid] - distribution[mid - 1] : distribution[mid];
                high = mid - 1;
            }
            else {
                low = mid + 1;
            }
        }

        Assert_(index != (uint)-1);
    }

    //=============================================================================================================================
    uint CalculateMarginalDensityFunctionCount(uint width, uint height)
    {
        return height;
    }

    //=============================================================================================================================
    uint CalculateConditionalDensityFunctionsCount(uint width, uint height)
    {
        return width * height;
    }

    //=============================================================================================================================
    void Ibl(const IblDensityFunctions* distributions, float r0, float r1, float& theta, float& phi, uint& x, uint& y, float& pdf)
    {
        // - http://www.igorsklyar.com/system/documents/papers/4/fiscourse.comp.pdf Section 4.2
        // - See also: Physically based rendering volume 2 section 13.6.5

        uint width = distributions->width;
        uint height = distributions->height;
        float widthf = (float)width;
        float heightf = (float)height;

        float mdf;
        float cdf;
        SampleDistributionFunction(distributions->marginalDensityFunction, height, r0, y, mdf);
        SampleDistributionFunction(distributions->conditionalDensityFunctions + y * width, width, r1, x, cdf);

        // -- theta represents the vertical position on the sphere and varies between 0 and pi
        theta = (y + 0.5f) * Math::Pi_ / heightf;

        // -- phi represents the horizontal position on the sphere and varies between -pi and pi
        phi = (x + 0.5f) * Math::TwoPi_ / widthf - Math::Pi_;

        // convert from texture space to spherical with the inverse of the Jacobian
        float invJacobian = (widthf * heightf) / Math::TwoPi_;

        // -- pdf is probably of x and y sample * sin(theta) to account for the warping along the y axis
        pdf = Math::Sinf(theta) * mdf * cdf * invJacobian;
    }

    //=============================================================================================================================
    float3 SampleIbl(const ImageBasedLightResourceData* ibl, float3 wi, float& pdf)
    {
        int32 width = (int32)ibl->densityfunctions.width;
        int32 height = (int32)ibl->densityfunctions.height;
        float widthf = (float)width;
        float heightf = (float)height;

        float theta;
        float phi;
        Math::NormalizedCartesianToSpherical(wi, theta, phi);

        // -- remap from [-pi, pi] to [0, 2pi]
        phi += Math::Pi_;

        int32 x = Clamp<int32>((int32)(phi * widthf / Math::TwoPi_ - 0.5f), 0, width);
        int32 y = Clamp<int32>((int32)(theta * heightf / Math::Pi_ - 0.5f), 0, height);

        float mdf;
        float cdf;
        if(y > 0)
            mdf = ibl->densityfunctions.marginalDensityFunction[y] - ibl->densityfunctions.marginalDensityFunction[y - 1];
        else
            mdf = ibl->densityfunctions.marginalDensityFunction[y];

        if(x > 0)
            cdf = (ibl->densityfunctions.conditionalDensityFunctions + y * width)[x]
                - (ibl->densityfunctions.conditionalDensityFunctions + y * width)[x - 1];
        else
            cdf = (ibl->densityfunctions.conditionalDensityFunctions + y * width)[x];

        // convert from texture space to spherical with the inverse of the Jacobian
        float invJacobian = (widthf * heightf) / Math::TwoPi_;

        // -- pdf is probably of x and y sample * sin(theta) to account for the warping along the y axis
        pdf = Math::Sinf(theta) * mdf * cdf * invJacobian;

        return ibl->hdrData[y * ibl->densityfunctions.width + x];
    }

    //=============================================================================================================================
    float SampleIBlPdf(const ImageBasedLightResourceData* ibl, float3 wi)
    {
        int32 width = (int32)ibl->densityfunctions.width;
        int32 height = (int32)ibl->densityfunctions.height;
        float widthf = (float)width;
        float heightf = (float)height;

        float theta;
        float phi;
        Math::NormalizedCartesianToSpherical(wi, theta, phi);

        // -- remap from [-pi, pi] to [0, 2pi]
        phi += Math::Pi_;

        int32 x = Clamp<int32>((int32)(phi * widthf / Math::TwoPi_ - 0.5f), 0, width);
        int32 y = Clamp<int32>((int32)(theta * heightf / Math::Pi_ - 0.5f), 0, height);

        float mdf;
        float cdf;
        if(y > 0)
            mdf = ibl->densityfunctions.marginalDensityFunction[y] - ibl->densityfunctions.marginalDensityFunction[y - 1];
        else
            mdf = ibl->densityfunctions.marginalDensityFunction[y];

        if(x > 0) {
            cdf = (ibl->densityfunctions.conditionalDensityFunctions + y * width)[x]
                - (ibl->densityfunctions.conditionalDensityFunctions + y * width)[x - 1];
        }
        else
            cdf = (ibl->densityfunctions.conditionalDensityFunctions + y * width)[x];

        // convert from texture space to spherical with the inverse of the Jacobian
        float invJacobian = (widthf * heightf) / Math::TwoPi_;

        // -- pdf is probably of x and y sample * sin(theta) to account for the warping along the y axis
        return Math::Sinf(theta) * mdf * cdf * invJacobian;
    }

    //=============================================================================================================================
    float3 SampleIbl(const ImageBasedLightResourceData* ibl, uint x, uint y)
    {
        return ibl->hdrData[y * ibl->densityfunctions.width + x];
    }

    //=============================================================================================================================
    void ShutdownDensityFunctions(IblDensityFunctions* distributions)
    {
        SafeFree_(distributions->conditionalDensityFunctions);
        SafeFree_(distributions->marginalDensityFunction);
    }
}