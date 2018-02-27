#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib/FloatStructs.h>
#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    struct TextureResourceData
    {
        static const uint MaxMipCount = 16;

        uint32 mipCount;
        uint32 dataSize;

        uint32 mipWidths[MaxMipCount];
        uint32 mipHeights[MaxMipCount];
        uint64 mipOffsets[MaxMipCount];

        // -- 0 is most detailed level
        float3* mipmaps;
    };

    struct TextureResource
    {
        TextureResourceData* data;
    };

    bool ReadTextureResource(cpointer filepath, TextureResource* texture);

    void DebugWriteTextureMips(TextureResource* texture, cpointer folder);
}