#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "MathLib/FloatStructs.h"
#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    struct TextureResourceData
    {
        enum TextureDataType
        {
            // -- Convert to something more like d3d formats?
            Float,
            Float3
        };

        static const uint MaxMipCount = 16;

        uint32 mipCount;
        uint32 dataSize;

        uint32 mipWidths[MaxMipCount];
        uint32 mipHeights[MaxMipCount];
        uint64 mipOffsets[MaxMipCount];

        TextureDataType type;
        uint32 pad;

        uint8* texture;
    };

    struct TextureResource
    {
        TextureResourceData* data;
    };

    Error ReadTextureResource(cpointer filepath, TextureResource* texture);
    void ShutdownTextureResource(TextureResource* texture);
    void DebugWriteTextureMips(TextureResource* texture, cpointer folder, cpointer name);
}