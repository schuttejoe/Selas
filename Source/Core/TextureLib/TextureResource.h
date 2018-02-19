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
        uint32 width;
        uint32 height;
        uint32 mipCount;
        uint32 dataSize;

        // -- 0 is most detailed level
        float3* mipmaps;
    };

    struct TextureResource
    {
        TextureResourceData* data;
    };

    bool ReadTextureResource(cpointer filepath, TextureResource* texture);
}