//==============================================================================
// Joe Schutte
//==============================================================================

#include "BuildCommon/BuildTexture.h"
#include "BuildCore/BuildContext.h"
#include "TextureLib/StbImage.h"
#include "TextureLib/TextureResource.h"
#include "UtilityLib/Color.h"
#include "StringLib/FixedString.h"
#include "StringLib/StringUtil.h"
#include "MathLib/ColorSpace.h"
#include "MathLib/FloatFuncs.h"
#include "SystemLib/BasicTypes.h"
#include "SystemLib/JsAssert.h"
#include "SystemLib/MemoryAllocation.h"
#include "SystemLib/Memory.h"
#include "SystemLib/MinMax.h"
#include "SystemLib/Logging.h"

#include <stdio.h>

namespace Selas
{
    //==============================================================================
    static void Uint32ToLinearFloat4(bool isLinear, uint width, uint height, uint32* rawData, float4* output)
    {
        uint count = width * height;
   
        for(uint scan = 0; scan < count; ++scan) {
            uint32 sample = rawData[scan];

            float4 linear = isLinear ? MakeColor4f(sample) : Math::SrgbToLinearPrecise(MakeColor4f(sample));
            output[scan] = linear;
        }
    }

    //==============================================================================
    static void Uint8ToLinearFloat(bool isLinear, uint width, uint height, uint8* rawData, float* output)
    {
        uint count = width * height;

        for(uint scan = 0; scan < count; ++scan) {
            uint8 sample = rawData[scan];
            float samplef = sample / 255.0f;

            float linear = isLinear ? samplef : Math::SrgbToLinearPrecise(samplef);
            output[scan] = linear;
        }
    }

    //==============================================================================
    static void Uint24ToLinearFloat3(bool isLinear, uint width, uint height, uint8* rawData, float3* output)
    {
        uint count = width * height;

        for(uint scan = 0; scan < count; ++scan) {
            uint8 r = rawData[3 * scan + 0];
            uint8 g = rawData[3 * scan + 1];
            uint8 b = rawData[3 * scan + 2];

            float3 linear = isLinear ? MakeColor3f(r, g, b) : Math::SrgbToLinearPrecise(MakeColor3f(r, g, b));
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
    static Error ConvertToLinearFloatData(void* rawData, uint width, uint height, float*& output)
    {
        uint count = width * height;
        output = AllocArray_(float, count);

        Uint8ToLinearFloat(true, width, height, (uint8*)rawData, output);

        return Success_;
    }

    //==============================================================================
    static Error ConvertToLinearFloat3Data(void* rawData, uint width, uint height, bool floatData, bool isSrcSrgb, float3*& output)
    {
        uint count = width * height;
        output = AllocArray_(float3, count);

        if(floatData) {
            Memory::Copy(output, rawData, sizeof(float3)*width*height);           
        }
        else {
            Uint24ToLinearFloat3(!isSrcSrgb, width, height, (uint8*)rawData, output);
        }

        return Success_;
    }

    //==============================================================================
    static Error ConvertToLinearFloat4Data(void* rawData, uint width, uint height, bool floatData, bool isSrcSrgb, float4*& output)
    {
        uint count = width * height;
        output = AllocArray_(float4, count);

        if(floatData) {
            Memory::Copy(output, rawData, sizeof(float4)*width*height);
        }
        else {
            Uint32ToLinearFloat4(!isSrcSrgb, width, height, (uint32*)rawData, output);
        }

        return Success_;
    }

    //==============================================================================
    template <typename Type_>
    static void BoxFilterMip(Type_* srcMip, uint srcWidth, uint srcHeight, Type_* dstMip, uint dstWidth, uint dstHeight)
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
    template <typename Type_>
    static bool GenerateMipMaps(TextureMipFilters prefilter, Type_* linear, uint width, uint height,
                                uint64* mipOffsets, uint32* mipWidths, uint32* mipHeights, Type_*& mipmaps, uint32& mipCount, uint32& dataSize)
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

        mipmaps = AllocArray_(Type_, totalTexelCount);
        dataSize = (uint32)(sizeof(Type_) * totalTexelCount);

        // -- copy top level
        Memory::Copy(mipmaps, linear, sizeof(Type_)*width*height);

        mipWidths[0] = (uint32)width;
        mipHeights[0] = (uint32)height;
        mipOffsets[0] = 0;

        // -- Filter remaining mips
        uint indexOffset = 0;
        for(uint scan = 1; scan < mipCount; ++scan) {

            uint srcWidth  = Max<uint>(width >> (scan-1), 1);
            uint srcHeight = Max<uint>(height >> (scan-1), 1);
            uint dstWidth  = Max<uint>(srcWidth >> 1, 1);
            uint dstHeight = Max<uint>(srcHeight >> 1, 1);

            uint srcTexelCount = srcWidth * srcHeight;

            switch(prefilter) {
            case Box:
                BoxFilterMip(mipmaps + indexOffset, srcWidth, srcHeight, mipmaps + indexOffset + srcTexelCount, dstWidth, dstHeight);
            };

            indexOffset += srcTexelCount;
            mipWidths[scan] = (uint32)dstWidth;
            mipHeights[scan] = (uint32)dstHeight;
            mipOffsets[scan] = sizeof(Type_) * indexOffset;
        }

        return true;
    }

    //==============================================================================
    bool IsNormalMapTexture(const FilePathString& str)
    {
        if(StringUtil::FindSubString(str.Ascii(), "_Normal") != nullptr) {
            return true;
        }

        if(StringUtil::FindSubString(str.Ascii(), "N_") != nullptr) {
            return true;
        }

        return false;
    }

    //==============================================================================
    Error ImportTexture(BuildProcessorContext* context, TextureMipFilters prefilter, TextureResourceData* texture)
    {
        FilePathString filepath;
        AssetFileUtils::ContentFilePath(context->source.name.Ascii(), filepath);
        context->AddFileDependency(filepath.Ascii());

        //LoadLinearFloaData
        FixedString32 extension;
        StringUtil::GetExtension(filepath.Ascii(), extension.Ascii(), (uint32)extension.Capacity());

        uint width;
        uint height;
        uint channels;
        bool floatData;
        void* rawData;
        ReturnError_(StbImageRead(filepath.Ascii(), NoComponentCountRequest_, width, height, channels, floatData, rawData));

        bool result;
        if(channels == 1) {
            float* linear = nullptr;
            ReturnError_(ConvertToLinearFloatData(rawData, width, height, linear));

            float* textureData;
            texture->dataSize = 0;
            result = GenerateMipMaps<float>(prefilter, linear, width, height,
                                     texture->mipOffsets, texture->mipWidths, texture->mipHeights, textureData, texture->mipCount, texture->dataSize);
            texture->texture = reinterpret_cast<uint8*>(textureData);
            texture->format = TextureResourceData::Float;
            Free_(linear);
        }
        else if (channels == 3) {

            bool isSrcSrgb = IsNormalMapTexture(filepath) == false;

            float3* linear = nullptr;
            ReturnError_(ConvertToLinearFloat3Data(rawData, width, height, floatData, isSrcSrgb, linear));

            float3* textureData;
            texture->dataSize = 0;
            result = GenerateMipMaps<float3>(prefilter, linear, width, height,
                                     texture->mipOffsets, texture->mipWidths, texture->mipHeights, textureData, texture->mipCount, texture->dataSize);
            texture->texture = reinterpret_cast<uint8*>(textureData);
            texture->format = TextureResourceData::Float3;
            Free_(linear);
        }
        else if(channels == 4) {

            bool isSrcSrgb = true;

            float4* linear = nullptr;
            ReturnError_(ConvertToLinearFloat4Data(rawData, width, height, floatData, isSrcSrgb, linear));

            float4* textureData;
            texture->dataSize = 0;
            result = GenerateMipMaps<float4>(prefilter, linear, width, height,
                                             texture->mipOffsets, texture->mipWidths, texture->mipHeights, textureData, texture->mipCount, texture->dataSize);
            texture->texture = reinterpret_cast<uint8*>(textureData);
            texture->format = TextureResourceData::Float4;
            Free_(linear);
        }
        else {
            return Error_("NYI - Unsupported (or NYI) channel texture format for texture '%s'.", filepath.Ascii());
        }

        Free_(rawData);

        return Success_;
    }
}
