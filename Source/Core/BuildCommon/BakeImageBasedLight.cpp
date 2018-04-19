//==============================================================================
// Joe Schutte
//==============================================================================

#include <BuildCommon/BakeImageBasedLight.h>
#include <SceneLib/ImageBasedLightResource.h>
#include <MathLib/ImportanceSampling.h>
#include <IoLib/BinarySerializer.h>

namespace Shooty
{
    //==============================================================================
    bool BakeImageBasedLight(const ImageBasedLightResourceData* data, cpointer filepath)
    {
        BinaryWriter writer;
        ReturnFailure_(SerializerStart(&writer, filepath));

        uint64 width = data->densityfunctions.width;
        uint64 height = data->densityfunctions.height;

        uint marginalDensityFunctionSize = sizeof(float) * ImportanceSampling::CalculateMarginalDensityFunctionCount(width, height);
        uint conditionalDensityFunctionSize = sizeof(float) * ImportanceSampling::CalculateConditionalDensityFunctionsCount(width, height);
        uint hdrDataSize = sizeof(float3) * width * height;

        SerializerWrite(&writer, &width, sizeof(width));
        SerializerWrite(&writer, &height, sizeof(height));

        SerializerWritePointerOffsetX64(&writer);
        SerializerWritePointerData(&writer, data->densityfunctions.marginalDensityFunction, marginalDensityFunctionSize);
        SerializerWritePointerOffsetX64(&writer);
        SerializerWritePointerData(&writer, data->densityfunctions.conditionalDensityFunctions, conditionalDensityFunctionSize);
        SerializerWritePointerOffsetX64(&writer);
        SerializerWritePointerData(&writer, data->hdrData, hdrDataSize);

        ReturnFailure_(SerializerEnd(&writer));

        return true;
    }
}