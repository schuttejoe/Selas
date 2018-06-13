//==============================================================================
// Joe Schutte
//==============================================================================

#include "BuildCommon/BakeTexture.h"
#include "BuildCore/BuildContext.h"
#include "TextureLib/TextureResource.h"
#include "StringLib/FixedString.h"
#include "IoLib/BinarySerializer.h"

#include <stdio.h>

namespace Selas
{
    //==============================================================================
    Error BakeTexture(BuildProcessorContext* context, const TextureResourceData* data)
    {
        uint32 pad = 0;

        FixedString256 typelessName;
        typelessName.Copy(context->source.name.Ascii());
        StringUtil::RemoveExtension(typelessName.Ascii());

        BinaryWriter writer;
        SerializerStart(&writer, data->dataSize + sizeof(*data));

        SerializerWrite(&writer, &data->mipCount, sizeof(data->mipCount));
        SerializerWrite(&writer, &data->dataSize, sizeof(data->dataSize));

        SerializerWrite(&writer, &data->mipWidths, sizeof(data->mipWidths));
        SerializerWrite(&writer, &data->mipHeights, sizeof(data->mipHeights));
        SerializerWrite(&writer, &data->mipOffsets, sizeof(data->mipOffsets));

        SerializerWrite(&writer, &data->type, sizeof(data->type));
        SerializerWrite(&writer, &pad, sizeof(pad));

        SerializerWritePointerOffsetX64(&writer);
        SerializerWritePointerData(&writer, data->texture, data->dataSize);

        void* rawData;
        uint32 rawSize;
        ReturnError_(SerializerEnd(&writer, rawData, rawSize));

        context->CreateOutput(TextureResource::kDataType, TextureResource::kDataVersion, context->source.name.Ascii(),
                               rawData, rawSize);

        Free_(rawData);

        return Success_;
    }
}