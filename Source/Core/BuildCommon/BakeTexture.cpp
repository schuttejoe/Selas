//==============================================================================
// Joe Schutte
//==============================================================================

#include <BuildCommon/BakeTexture.h>
#include <TextureLib/TextureResource.h>
#include <StringLib/FixedString.h>
#include <IoLib/BinarySerializer.h>

#include <stdio.h>

namespace Selas
{
    cpointer TextureAssetDirectory = "D:\\Shooty\\Selas\\_Assets\\Textures\\";

    //==============================================================================
    Error BakeTexture(const TextureResourceData* data, cpointer textureName)
    {
        uint32 pad = 0;

        FixedString256 typelessName;
        typelessName.Copy(textureName);
        StringUtil::RemoveExtension(typelessName.Ascii());

        // JSTODO - Unify all assets using .bin extension
        FixedString512 filepath;
        #if IsWindows_
            sprintf_s(filepath.Ascii(), filepath.Capcaity(), "%s%s.bin", TextureAssetDirectory, typelessName.Ascii());
        #else
            sprintf(filepath.Ascii(), "%s%s.bin", TextureAssetDirectory, typelessName.Ascii());
        #endif

        BinaryWriter writer;
        ReturnError_(SerializerStart(&writer, filepath.Ascii(), data->dataSize + sizeof(*data)));

        SerializerWrite(&writer, &data->mipCount, sizeof(data->mipCount));
        SerializerWrite(&writer, &data->dataSize, sizeof(data->dataSize));

        SerializerWrite(&writer, &data->mipWidths, sizeof(data->mipWidths));
        SerializerWrite(&writer, &data->mipHeights, sizeof(data->mipHeights));
        SerializerWrite(&writer, &data->mipOffsets, sizeof(data->mipOffsets));

        SerializerWrite(&writer, &data->type, sizeof(data->type));
        SerializerWrite(&writer, &pad, sizeof(pad));

        SerializerWritePointerOffsetX64(&writer);
        SerializerWritePointerData(&writer, data->texture, data->dataSize);

        ReturnError_(SerializerEnd(&writer));

        return Success_;
    }
}