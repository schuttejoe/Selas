//==============================================================================
// Joe Schutte
//==============================================================================

#include "BuildCommon/BakeImageBasedLight.h"
#include "BuildCore/BuildContext.h"
#include "SceneLib/ImageBasedLightResource.h"
#include "MathLib/ImportanceSampling.h"
#include "IoLib/BinarySerializer.h"

namespace Selas
{
    //==============================================================================
    Error BakeImageBasedLight(BuildProcessorContext* context, const ImageBasedLightResourceData* data)
    {
        uint64 width = data->densityfunctions.width;
        uint64 height = data->densityfunctions.height;

        uint marginalDensityFunctionSize = sizeof(float) * CalculateMarginalDensityFunctionCount(width, height);
        uint conditionalDensityFunctionSize = sizeof(float) * CalculateConditionalDensityFunctionsCount(width, height);
        uint hdrDataSize = sizeof(float3) * width * height;

        BinaryWriter writer;
        SerializerStart(&writer, 64, (uint32)(marginalDensityFunctionSize + conditionalDensityFunctionSize + hdrDataSize));

        SerializerWrite(&writer, &width, sizeof(width));
        SerializerWrite(&writer, &height, sizeof(height));

        SerializerWritePointerOffsetX64(&writer);
        SerializerWritePointerData(&writer, data->densityfunctions.marginalDensityFunction, marginalDensityFunctionSize);
        SerializerWritePointerOffsetX64(&writer);
        SerializerWritePointerData(&writer, data->densityfunctions.conditionalDensityFunctions, conditionalDensityFunctionSize);
        SerializerWritePointerOffsetX64(&writer);
        SerializerWritePointerData(&writer, data->hdrData, hdrDataSize);

        void* rawData;
        uint32 dataSize;
        ReturnError_(SerializerEnd(&writer, rawData, dataSize));

        ReturnError_(context->CreateOutput(ImageBasedLightResource::kDataType, ImageBasedLightResource::kDataVersion,
                                           context->source.name.Ascii(), rawData, dataSize));

        Free_(rawData);

        return Success_;
    }
}