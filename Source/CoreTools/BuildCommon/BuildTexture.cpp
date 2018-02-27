//==============================================================================
// Joe Schutte
//==============================================================================

#include <BuildCommon/BuildTexture.h>
#include <TextureLib/StbImage.h>
#include <TextureLib/TextureResource.h>
#include <UtilityLib/Color.h>
#include <StringLib/StringUtil.h>
#include <MathLib/ColorSpace.h>
#include <MathLib/FloatFuncs.h>
#include <SystemLib/BasicTypes.h>
#include <SystemLib/JsAssert.h>
#include <SystemLib/MemoryAllocation.h>
#include <SystemLib/Memory.h>
#include <SystemLib/MinMax.h>

namespace Shooty
{
    //==============================================================================
    static void Uint32ToLinearFloat3(bool isLinear, uint width, uint height, uint32* rawData, float3* output)
    {
        uint count = width * height;
   
        for(uint scan = 0; scan < count; ++scan) {
            uint32 sample = rawData[scan];

            float3 linear = isLinear ? MakeColor3f(sample) : Math::SrgbToLinearPrecise(MakeColor3f(sample));
            output[scan] = linear;
        }
    }

    //==============================================================================
    static void Float4ToLinearFloat3(bool isLinear, uint width, uint height, float4* rawData, float3* output)
    {
        uint count = width * height;

        for(uint scan = 0; scan < count; ++scan) {
            float4 sample = rawData[scan];
            float3 rgb = float3(sample.x, sample.y, sample.z);

            float3 linear = isLinear ? rgb : Math::SrgbToLinearPrecise(rgb);
            output[scan] = linear;
        }
    }

    //==============================================================================
    static bool LoadLinearFloat3Data(const char* filepath, float3*& output, uint& width, uint& height)
    {
        uint channels;
        void* rawData;
        ReturnFailure_(StbImageRead(filepath, width, height, channels, rawData));

        uint count = width * height;
        output = AllocArray_(float3, count);

        bool isHdr = StringUtil::EndsWithIgnoreCase(filepath, "hdr");
        if(isHdr) {
            if(channels == 3) {
                Memory::Copy(output, rawData, sizeof(float3)*width*height);
            }
            else if(channels == 4) {
                Float4ToLinearFloat3(true, width, height, (float4*)rawData, output);
            }
            else {
                Free_(rawData);
                return false;
            }
        }
        else {
            Uint32ToLinearFloat3(false, width, height, (uint32*)rawData, output);
        }
        Free_(rawData);

        return true;
    }

    //==============================================================================
    static void BoxFilterMip(float3* srcMip, uint srcWidth, uint srcHeight, float3* dstMip, uint dstWidth, uint dstHeight)
    {
        if(srcWidth == 1) {
            for(uint y = 0; y < dstHeight; ++y) {
                *dstMip = 0.5f * (srcMip[0] + srcMip[srcWidth]);
                srcMip += srcWidth;
            }
            return;
        }

        if(srcHeight == 1) {
            for(uint x = 0; x < dstHeight; ++x) {
                *dstMip = 0.5f * (srcMip[0] + srcMip[1]);
            }
            return;
        }

        for(uint y = 0; y < dstHeight; ++y) {
            for(uint x = 0; x < dstWidth; ++x) {
                *dstMip = 0.25f * (srcMip[0] + srcMip[1] + srcMip[srcWidth] + srcMip[srcWidth + 1]);
                dstMip++;
                srcMip += 2;
            }

            srcMip += srcWidth;
        }
    }

    //==============================================================================
    static bool GenerateMipMaps(TextureMipFilters prefilter, float3* linear, uint width, uint height,
                                uint64* mipOffsets, uint32* mipWidths, uint32* mipHeights, float3*& mipmaps, uint32& mipCount, uint32& dataSize)
    {
        AssertMsg_(prefilter == Box, "Only Box filter is currently implemented");

        uint mipWidth  = width;
        uint mipHeight = height;

        uint totalTexelCount = 1;
        mipCount = 1;
        while(mipWidth > 1 || mipHeight > 1) {
            uint mipTexelCount = mipWidth * mipHeight;
            totalTexelCount += mipTexelCount;

            mipWidth  = Max<uint>(mipWidth >> 1, 1);
            mipHeight = Max<uint>(mipHeight >> 1, 1);
            ++mipCount;
        }

        if(mipCount > TextureResourceData::MaxMipCount) {
            return false;
        }

        mipmaps = AllocArray_(float3, totalTexelCount);
        dataSize = (uint32)(sizeof(float3) * totalTexelCount);

        // -- copy top level
        Memory::Copy(mipmaps, linear, sizeof(float3)*width*height);

        // -- Filter remaining mips
        uint offset = 0;
        for(uint scan = 0; scan < mipCount; ++scan) {

            uint srcWidth  = Max<uint>(width >> scan, 1);
            uint srcHeight = Max<uint>(height >> scan, 1);
            uint dstWidth  = Max<uint>(srcWidth >> 1, 1);
            uint dstHeight = Max<uint>(srcHeight >> 1, 1);

            uint srcTexelCount = srcWidth * srcHeight;
            uint dstTexelCount = dstWidth * dstHeight;

            switch(prefilter) {
            case Box:
                BoxFilterMip(mipmaps + offset, srcWidth, srcHeight, mipmaps + offset + srcTexelCount, dstWidth, dstHeight);
            };

            mipWidths[scan] = (uint32)srcWidth;
            mipHeights[scan] = (uint32)srcHeight;
            mipOffsets[scan] = offset;
            offset += srcTexelCount;
        }

        return true;
    }

    //==============================================================================
    bool ImportTexture(const char* filepath, TextureMipFilters prefilter, TextureResourceData* texture)
    {
        float3* linear = nullptr;
        uint width;
        uint height;
        ReturnFailure_(LoadLinearFloat3Data(filepath, linear, width, height));

        texture->dataSize = 0;
        bool success = GenerateMipMaps(prefilter, linear, width, height,
                                       texture->mipOffsets, texture->mipWidths, texture->mipHeights, texture->mipmaps, texture->mipCount, texture->dataSize);
        Free_(linear);

        return success;
    }
}