//==============================================================================
// Joe Schutte
//==============================================================================

#include <BuildCommon/BuildTexture.h>
#include <TextureLib/StbImage.h>
#include <TextureLib/TextureResource.h>
#include <UtilityLib/Color.h>
#include <StringLib/FixedString.h>
#include <StringLib/StringUtil.h>
#include <MathLib/ColorSpace.h>
#include <MathLib/FloatFuncs.h>
#include <SystemLib/BasicTypes.h>
#include <SystemLib/JsAssert.h>
#include <SystemLib/MemoryAllocation.h>
#include <SystemLib/Memory.h>
#include <SystemLib/MinMax.h>

#include <stdio.h>

namespace Selas
{
    cpointer TextureBaseDirectory = "D:\\Shooty\\Selas\\Content\\Textures\\";

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
    static void Uint24ToLinearFloat(bool isLinear, uint width, uint height, uint8* rawData, float* output)
    {
        uint count = width * height;

        for(uint scan = 0; scan < count; ++scan) {
            uint8 r = rawData[3 * scan + 0];

            float linear = isLinear ? r : Math::SrgbToLinearPrecise(r);
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
    static bool LoadLinearFloatData(const char* filepath, float*& output, uint& width, uint& height)
    {
        uint channels;
        void* rawData;
        ReturnFailure_(StbImageRead(filepath, 1, width, height, channels, rawData));

        uint count = width * height;
        output = AllocArray_(float, count);

        if(channels == 1) {
            Uint8ToLinearFloat(true, width, height, (uint8*)rawData, output);
        }
        else if(channels == 3) {
            // -- stb reports the original but will successfully convert to 1 channel in that case.
            // -- verified for jpg at least
            Uint8ToLinearFloat(true, width, height, (uint8*)rawData, output);
        }
        else {
            Free_(rawData);
            return false;
        }

        Free_(rawData);

        return true;
    }

    //==============================================================================
    static bool LoadLinearFloat3Data(const char* filepath, bool isSrcSrgb, float3*& output, uint& width, uint& height)
    {
        uint channels;
        void* rawData;
        ReturnFailure_(StbImageRead(filepath, 3, width, height, channels, rawData));

        uint count = width * height;
        output = AllocArray_(float3, count);

        bool isHdr = StringUtil::EndsWithIgnoreCase(filepath, "hdr");
        if(isHdr) {
            if(channels == 3) {
                Memory::Copy(output, rawData, sizeof(float3)*width*height);
            }
            else if(channels == 4) {
                Float4ToLinearFloat3(false, width, height, (float4*)rawData, output);
            }
            else {
                Free_(rawData);
                return false;
            }
        }
        else {
            if(channels == 3) {
                Uint24ToLinearFloat3(!isSrcSrgb, width, height, (uint8*)rawData, output);
            }
            else if(channels == 4) {
                Uint32ToLinearFloat3(!isSrcSrgb, width, height, (uint32*)rawData, output);
            }
            else {
                Free_(rawData);
                return false;
            }
            
        }
        Free_(rawData);

        return true;
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
            uint dstTexelCount = dstWidth * dstHeight;

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

    enum MaterialTextureTypes
    {
        AO,
        Albedo,
        Height,
        Normal,
        Roughness,
        ReflectanceAtR0,
        Metalness,

        Unknown
    };

    //==============================================================================
    static MaterialTextureTypes DetermineMaterialType(cpointer textureName)
    {
        if(StringUtil::FindSubString(textureName, "_AO")) {
            return AO;
        }
        if(StringUtil::FindSubString(textureName, "_Albedo")) {
            return Albedo;
        }
        if(StringUtil::FindSubString(textureName, "_Height")) {
            return Height;
        }
        if(StringUtil::FindSubString(textureName, "_Normal")) {
            return Normal;
        }
        if(StringUtil::FindSubString(textureName, "_Roughness")) {
            return Roughness;
        }
        if(StringUtil::FindSubString(textureName, "_Specular")) {
            return ReflectanceAtR0;
        }
        if(StringUtil::FindSubString(textureName, "_Metalness")) {
            return Metalness;
        }

        return Unknown;
    }

    //==============================================================================
    bool ImportTexture(cpointer textureName, TextureMipFilters prefilter, TextureResourceData* texture)
    {
        FixedString512 filepath;
        sprintf_s(filepath.Ascii(), filepath.Capcaity(), "%s%s", TextureBaseDirectory, textureName);

        //LoadLinearFloaData
        FixedString32 extension;
        StringUtil::GetExtension(filepath.Ascii(), extension.Ascii(), (uint32)extension.Capcaity());

        MaterialTextureTypes type = DetermineMaterialType(textureName);
        Assert_(type != Unknown);

        bool result;
        if(type == AO || type == Height || type == Roughness || type == Metalness) {
            float* linear = nullptr;
            uint width;
            uint height;
            ReturnFailure_(LoadLinearFloatData(filepath.Ascii(), linear, width, height));

            float* textureData;
            texture->dataSize = 0;
            result = GenerateMipMaps<float>(prefilter, linear, width, height,
                                     texture->mipOffsets, texture->mipWidths, texture->mipHeights, textureData, texture->mipCount, texture->dataSize);
            texture->texture = reinterpret_cast<uint8*>(textureData);
            texture->type = TextureResourceData::Float;
            Free_(linear);
        }
        else if (type == Albedo || type == ReflectanceAtR0 || type == Normal) {

            bool isSrcSrgb = type == Normal ? false : true;

            float3* linear = nullptr;
            uint width;
            uint height;
            ReturnFailure_(LoadLinearFloat3Data(filepath.Ascii(), isSrcSrgb, linear, width, height));

            float3* textureData;
            texture->dataSize = 0;
            result = GenerateMipMaps<float3>(prefilter, linear, width, height,
                                     texture->mipOffsets, texture->mipWidths, texture->mipHeights, textureData, texture->mipCount, texture->dataSize);
            texture->texture = reinterpret_cast<uint8*>(textureData);
            texture->type = TextureResourceData::Float3;
            Free_(linear);
        }


        return result;
    }
}