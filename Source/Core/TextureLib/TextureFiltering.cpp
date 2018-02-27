//==============================================================================
// Joe Schutte
//==============================================================================

#include <TextureLib/TextureFiltering.h>
#include <TextureLib/TextureResource.h>
#include <MathLib/FloatFuncs.h>
#include <MathLib/IntStructs.h>
#include <MathLib/Trigonometric.h>
#include <SystemLib/BasicTypes.h>
#include <SystemLib/JsAssert.h>
#include <SystemLib/MinMax.h>

namespace Shooty
{
    // JSTODO - Credit to pbrt

	const uint EwaLutSize = 128;
	static float EWAFilterLut[EwaLutSize];

	//==============================================================================
    namespace TextureFiltering
    {
        //==============================================================================
        void InitializeEWAFilterWeights()
        {
            for(uint i = 0; i < EwaLutSize; ++i) {
                float alpha = 2.0f;
                float r2 = float(i) / (EwaLutSize - 1.0f);
                EWAFilterLut[i] = Math::Expf(-alpha * r2) - Math::Expf(-alpha);
            }
        }

        //==============================================================================
        float3 Sample(float3* mip, WrapMode wrapMode, uint32 w, uint32 h, int32 s, int32 t)
        {
            switch(wrapMode) {
            case WrapMode::Clamp:
                s = Shooty::Clamp<int32>(s, 0, w - 1);
                t = Shooty::Clamp<int32>(t, 0, h - 1);
                break;
            case WrapMode::Repeat:
                s = s % w;
                t = t % h;
                break;
            default:
                Assert_(false);
            }

            return mip[t * w + s];
        }

        //==============================================================================
        float3 Point(TextureResourceData* texture, float2 st)
        {
            uint level = 0;

            WrapMode wrapMode = WrapMode::Repeat;

            uint64 mipOffset = texture->mipOffsets[level];
            uint32 mipWidth = texture->mipWidths[level];
            uint32 mipHeight = texture->mipHeights[level];

            float3* mip = &texture->mipmaps[mipOffset];

            float s = st.x * mipWidth;
            float t = st.y * mipHeight;
            int32 s0 = (int32)Math::Floor(s);
            int32 t0 = (int32)Math::Floor(t);

            return Sample(mip, wrapMode, mipWidth, mipHeight, s0, t0);
        }

        //==============================================================================
        float3 Triangle(TextureResourceData* texture, int32 level, float2 st)
        {
            level = Min<uint32>(level, texture->mipCount - 1);

            WrapMode wrapMode = WrapMode::Repeat;

            uint64 mipOffset = texture->mipOffsets[level];
            uint32 mipWidth = texture->mipWidths[level];
            uint32 mipHeight = texture->mipHeights[level];

            float3* mip = &texture->mipmaps[mipOffset];

            float s = st.x * mipWidth  - 0.5f;
            float t = st.y * mipHeight - 0.5f;

            int32 s0 = (int32)Math::Floor(s);
            int32 t0 = (int32)Math::Floor(t);
            float ds = s - s0;
            float dt = t - t0;
            return (1 - ds) * (1 - dt) * Sample(mip, wrapMode, mipWidth, mipHeight, s0, t0) +
                   (1 - ds) *      dt  * Sample(mip, wrapMode, mipWidth, mipHeight, s0, t0 + 1) +
                        ds  * (1 - dt) * Sample(mip, wrapMode, mipWidth, mipHeight, s0 + 1, t0) +
                        ds  *      dt  * Sample(mip, wrapMode, mipWidth, mipHeight, s0 + 1, t0 + 1);
        }

        //==============================================================================
        static float3 EWA(TextureResourceData* texture, int32 reqLevel, float2 st, float2 dst0, float2 dst1)
        {
            int32 level = Min<uint32>(reqLevel, texture->mipCount - 1);

            WrapMode wrapMode = WrapMode::Repeat;

            uint64 mipOffset = texture->mipOffsets[level];
            uint32 mipWidth = texture->mipWidths[level];
            uint32 mipHeight = texture->mipHeights[level];
            float3* mip = &texture->mipmaps[mipOffset];

            // Convert EWA coordinates to appropriate scale for level
            st.x = st.x * mipWidth - 0.5f;
            st.y = st.y * mipHeight - 0.5f;
            dst0.x *= mipWidth;
            dst0.y *= mipHeight;
            dst1.x *= mipWidth;
            dst1.y *= mipHeight;

            // Compute ellipse coefficients to bound EWA filter region
            float A = dst0.y * dst0.y + dst1.y * dst1.y + 1;
            float B = -2 * (dst0.x * dst0.y + dst1.x * dst1.y);
            float C = dst0.x * dst0.x + dst1.x * dst1.x + 1;
            float invF = 1 / (A * C - B * B * 0.25f);
            A *= invF;
            B *= invF;
            C *= invF;

            // Compute the ellipse's bounding box in texture space
            float det = -B * B + 4 * A * C;
            float invDet = 1 / det;
            float uSqrt = Math::Sqrtf(det * C), vSqrt = Math::Sqrtf(A * det);
            int32 s0 = (int32)Math::Ceil(st.x - 2 * invDet * uSqrt);
            int32 s1 = (int32)Math::Floor(st.x + 2 * invDet * uSqrt);
            int32 t0 = (int32)Math::Ceil(st.y - 2 * invDet * vSqrt);
            int32 t1 = (int32)Math::Floor(st.y + 2 * invDet * vSqrt);

            // Scan over ellipse bound and compute quadratic equation
            float3 sum = float3::Zero_;
            float sumWts = 0;
            for(int32 it = t0; it <= t1; ++it) {
                float tt = it - st.y;
                for(int32 is = s0; is <= s1; ++is) {
                    float ss = is - st.x;
                    // Compute squared radius and filter texel if inside ellipse
                    float r2 = A * ss * ss + B * ss * tt + C * tt * tt;
                    if(r2 < 1) {
                        int32 index = Min<int32>((int32)(r2 * EwaLutSize), EwaLutSize - 1);
                        float weight = EWAFilterLut[index];
                        sum += Sample(mip, wrapMode, mipWidth, mipHeight, is, it) * weight;
                        sumWts += weight;
                    }
                }
            }

            return sum * (1.0f / sumWts);
        }

        //==============================================================================
        float3 EWA(TextureResourceData* texture, float2 st, float2 dst0, float2 dst1)
        {
            // This was entirely copied from PBRT so 100% credit to them.

            // Compute ellipse minor and major axes
            if(LengthSquared(dst0) < LengthSquared(dst1)) {
                float2 tmp = dst0;
                dst0 = dst1;
                dst1 = tmp;
            }

            float majorLength = Length(dst0);
            float minorLength = Length(dst1);

            const uint maxAnisotropy = 16;

            // Clamp ellipse eccentricity if too large
            if(minorLength * maxAnisotropy < majorLength && minorLength > 0) {
                float scale = majorLength / (minorLength * maxAnisotropy);
                dst1 = dst1 * scale;
                minorLength *= scale;
            }
            if (minorLength == 0) 
                return Triangle(texture, 0, st);

            // Choose level of detail for EWA lookup and perform EWA filtering
            float lod = Max<float>(0.0f, texture->mipCount - 1.0f + Math::Log2(minorLength));
            float ilod = Math::Floor(lod);

            return Lerp(EWA(texture, (int32)ilod, st, dst0, dst1), EWA(texture, (int32)ilod + 1, st, dst0, dst1), lod - ilod);
        }
    }
}