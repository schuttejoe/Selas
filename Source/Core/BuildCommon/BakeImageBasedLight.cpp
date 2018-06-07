//==============================================================================
// Joe Schutte
//==============================================================================

#include "BuildCommon/BakeImageBasedLight.h"
#include "SceneLib/ImageBasedLightResource.h"
#include "MathLib/ImportanceSampling.h"
#include "IoLib/BinarySerializer.h"

namespace Selas
{
    //==============================================================================
    Error BakeImageBasedLight(const ImageBasedLightResourceData* data, cpointer filepath)
    {
        uint64 width = data->densityfunctions.width;
        uint64 height = data->densityfunctions.height;

        uint marginalDensityFunctionSize = sizeof(float) * CalculateMarginalDensityFunctionCount(width, height);
        uint conditionalDensityFunctionSize = sizeof(float) * CalculateConditionalDensityFunctionsCount(width, height);
        uint hdrDataSize = sizeof(float3) * width * height;

        BinaryWriter writer;
        ReturnError_(SerializerStart(&writer, filepath, 64, (uint32)(marginalDensityFunctionSize + conditionalDensityFunctionSize + hdrDataSize)));

        SerializerWrite(&writer, &width, sizeof(width));
        SerializerWrite(&writer, &height, sizeof(height));

        SerializerWritePointerOffsetX64(&writer);
        SerializerWritePointerData(&writer, data->densityfunctions.marginalDensityFunction, marginalDensityFunctionSize);
        SerializerWritePointerOffsetX64(&writer);
        SerializerWritePointerData(&writer, data->densityfunctions.conditionalDensityFunctions, conditionalDensityFunctionSize);
        SerializerWritePointerOffsetX64(&writer);
        SerializerWritePointerData(&writer, data->hdrData, hdrDataSize);

        ReturnError_(SerializerEnd(&writer));

        return Success_;
    }
}