//==============================================================================
// Joe Schutte
//==============================================================================

#include <BuildCommon/BakeTexture.h>
#include <TextureLib/TextureResource.h>
#include <IoLib/BinarySerializer.h>

namespace Shooty
{
    //==============================================================================
    bool BakeTexture(const TextureResourceData* data, cpointer filepath)
    {
        BinaryWriter writer;
        ReturnFailure_(SerializerStart(&writer, filepath));

        SerializerWrite(&writer, &data->width, sizeof(data->width));
        SerializerWrite(&writer, &data->height, sizeof(data->height));
        SerializerWrite(&writer, &data->mipCount, sizeof(data->mipCount));
        SerializerWrite(&writer, &data->dataSize, sizeof(data->dataSize));

        SerializerWritePointerOffsetX64(&writer);
        SerializerWritePointerData(&writer, data->mipmaps, data->dataSize);

        ReturnFailure_(SerializerEnd(&writer));

        return true;
    }
}