//==============================================================================
// Joe Schutte
//==============================================================================

#include <TextureLib/TextureFiltering.h>
#include <TextureLib/TextureResource.h>
#include <MathLib/FloatFuncs.h>
#include <MathLib/IntStructs.h>
#include <SystemLib/BasicTypes.h>
#include <SystemLib/JsAssert.h>

namespace Shooty
{
    //==============================================================================
    float3 PointSampleTexture(TextureResourceData* texture, float2 uvs)
    {
        uint32 x = (uint32)(uvs.x * texture->width);
        uint32 y = (uint32)(uvs.y * texture->height);

        Assert_(x >= 0 && x < texture->width);
        Assert_(y >= 0 && y < texture->height);

        return texture->mipmaps[y * texture->width + x];
    }
}