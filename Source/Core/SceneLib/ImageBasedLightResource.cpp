//==============================================================================
// Joe Schutte
//==============================================================================

#include <SceneLib/ImageBasedLightResource.h>
#include <IoLib/BinarySerializer.h>
#include <IoLib/File.h>
#include <MathLib/Projection.h>
#include <MathLib/Trigonometric.h>
#include <SystemLib/BasicTypes.h>
#include <SystemLib/MinMax.h>

namespace Shooty
{
    //==============================================================================
    bool ReadImageBasedLightResource(cpointer filepath, ImageBasedLightResource* resource)
    {

        void* fileData = nullptr;
        uint32 fileSize = 0;
        File::ReadWholeFile(filepath, &fileData, &fileSize);

        BinaryReader reader;
        SerializerStart(&reader, fileData, fileSize);

        SerializerAttach(&reader, reinterpret_cast<void**>(&resource->data), fileSize);

        FixupPointerX64(fileData, resource->data->densityfunctions.marginalDensityFunction);
        FixupPointerX64(fileData, resource->data->densityfunctions.conditionalDensityFunctions);
        FixupPointerX64(fileData, resource->data->hdrData);

        SerializerEnd(&reader);

        return true;
    }

    //==============================================================================
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

    //==============================================================================
    uint CalculateMarginalDensityFunctionCount(uint width, uint height)
    {
        return height;
    }

    //==============================================================================
    uint CalculateConditionalDensityFunctionsCount(uint width, uint height)
    {
        return width * height;
    }

    //==============================================================================
    float IblPdf(const IblDensityFunctions* distributions, float3 w)
    {
        int32 width = (int32)distributions->width;
        int32 height = (int32)distributions->height;
        float widthf = (float)width;
        float heightf = (float)height;

        float theta;
        float phi;
        Math::NormalizedCartesianToSpherical(w, theta, phi);

        // -- remap from [-pi, pi] to [0, 2pi]
        phi += Math::Pi_;

        int32 x = Clamp<int32>((int32)(phi * widthf / Math::TwoPi_ - 0.5f), 0, width);
        int32 y = Clamp<int32>((int32)(theta * heightf / Math::Pi_ - 0.5f), 0, height);

        float mdf = distributions->marginalDensityFunction[y];
        float cdf = (distributions->conditionalDensityFunctions + y * width)[x];

        // convert from texture space to spherical with the inverse of the Jacobian
        float invJacobian = (widthf * heightf) / Math::TwoPi_;

        // -- pdf is probably of x and y sample * sin(theta) to account for the warping along the y axis
        return Math::Sinf(theta) * mdf * cdf * invJacobian;
    }

    //==============================================================================
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

    //==============================================================================
    void ShutdownDensityFunctions(IblDensityFunctions* distributions)
    {
        SafeFree_(distributions->conditionalDensityFunctions);
        SafeFree_(distributions->marginalDensityFunction);
    }

    //==============================================================================
    float3 SampleIbl(const ImageBasedLightResourceData* ibl, float3 wi)
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

        return ibl->hdrData[y * ibl->densityfunctions.width + x];
    }
}