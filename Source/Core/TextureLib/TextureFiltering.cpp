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
        template <typename Type_>
        static Type_ Sample(Type_* mip, WrapMode wrapMode, uint32 w, uint32 h, int32 s, int32 t)
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
        template <typename Type_>
        static void Point(TextureResourceData* texture, float2 st, Type_& result)
        {
            uint level = 0;

            WrapMode wrapMode = WrapMode::Repeat;

            uint64 mipOffset = texture->mipOffsets[level];
            uint32 mipWidth  = texture->mipWidths[level];
            uint32 mipHeight = texture->mipHeights[level];

            Type_* mip = reinterpret_cast<Type_*>(&texture->texture[mipOffset]);

            float s = st.x * mipWidth;
            float t = st.y * mipHeight;
            int32 s0 = (int32)Math::Floor(s);
            int32 t0 = (int32)Math::Floor(t);

            result = Sample<Type_>(mip, wrapMode, mipWidth, mipHeight, s0, t0);
        }

        //==============================================================================
        template <typename Type_>
        void Triangle(TextureResourceData* texture, int32 level, float2 st, Type_& result)
        {
            level = Min<uint32>(level, texture->mipCount - 1);

            WrapMode wrapMode = WrapMode::Repeat;

            uint64 mipOffset = texture->mipOffsets[level];
            uint32 mipWidth = texture->mipWidths[level];
            uint32 mipHeight = texture->mipHeights[level];

            Type_* mip = reinterpret_cast<Type_*>(&texture->texture[mipOffset]);

            float s = st.x * mipWidth - 0.5f;
            float t = st.y * mipHeight - 0.5f;

            int32 s0 = (int32)Math::Floor(s);
            int32 t0 = (int32)Math::Floor(t);
            float ds = s - s0;
            float dt = t - t0;
            result = (1 - ds) * (1 - dt) * Sample<Type_>(mip, wrapMode, mipWidth, mipHeight, s0, t0) +
                     (1 - ds) *      dt  * Sample<Type_>(mip, wrapMode, mipWidth, mipHeight, s0, t0 + 1) +
                          ds  * (1 - dt) * Sample<Type_>(mip, wrapMode, mipWidth, mipHeight, s0 + 1, t0) +
                          ds  *      dt  * Sample<Type_>(mip, wrapMode, mipWidth, mipHeight, s0 + 1, t0 + 1);
        }

        //==============================================================================
        template <typename Type_>
        static void EWA(TextureResourceData* texture, int32 reqLevel, float2 st, float2 dst0, float2 dst1, Type_& result)
        {
            // Credit goes to pbrt for the EWA implementation
            // https://github.com/mmp/pbrt-v3

            WrapMode wrapMode = WrapMode::Repeat;

            if(reqLevel >= (int32)texture->mipCount) {
                uint64 mipOffset = texture->mipOffsets[texture->mipCount - 1];
                Type_* mip = reinterpret_cast<Type_*>(&texture->texture[mipOffset]);
                result = Sample<Type_>(mip, wrapMode, 1, 1, 0, 0);
                return;
            }

            uint64 mipOffset = texture->mipOffsets[reqLevel];
            uint32 mipWidth = texture->mipWidths[reqLevel];
            uint32 mipHeight = texture->mipHeights[reqLevel];
            Type_* mip = reinterpret_cast<Type_*>(&texture->texture[mipOffset]);

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
            float uSqrt = Math::Sqrtf(det * C);
            float vSqrt = Math::Sqrtf(A * det);
            int32 s0 = (int32)Math::Ceil(st.x - 2 * invDet * uSqrt);
            int32 s1 = (int32)Math::Floor(st.x + 2 * invDet * uSqrt);
            int32 t0 = (int32)Math::Ceil(st.y - 2 * invDet * vSqrt);
            int32 t1 = (int32)Math::Floor(st.y + 2 * invDet * vSqrt);

            // Scan over ellipse bound and compute quadratic equation
            Type_ sum = Type_(0.0f);
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
                        sum += Sample<Type_>(mip, wrapMode, mipWidth, mipHeight, is, it) * weight;
                        sumWts += weight;
                    }
                }
            }

            result = sum * (1.0f / sumWts);
        }

        //==============================================================================
        template <typename Type_>
        static void EWA(TextureResourceData* texture, float2 st, float2 dst0, float2 dst1, Type_& result)
        {
            // Credit goes to pbrt for the EWA implementation
            // https://github.com/mmp/pbrt-v3

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
            if(minorLength == 0) {
                Triangle<Type_>(texture, 0, st, result);
                return;
            }

            // Choose level of detail for EWA lookup and perform EWA filtering
            float lod = Max<float>(0.0f, texture->mipCount - 1.0f + Math::Log2(minorLength));
            float ilod = Math::Floor(lod);

            Type_ r0;
            EWA<Type_>(texture, (int32)ilod, st, dst0, dst1, r0);
            Type_ r1;
            EWA<Type_>(texture, (int32)ilod + 1, st, dst0, dst1, r1);
            result = Lerp(r0, r1, lod - ilod);
        }

        //==============================================================================
        float PointFloat(TextureResourceData* texture, float2 st)
        {
            Assert_(texture->type == TextureResourceData::Float);

            float result;
            Point(texture, st, result);
            return result;
        }

        //==============================================================================
        float TriangleFloat(TextureResourceData* texture, int32 level, float2 st)
        {
            Assert_(texture->type == TextureResourceData::Float);

            float result;
            Triangle(texture, level, st, result);
            return result;
        }

        //==============================================================================
        float EWAFloat(TextureResourceData* texture, float2 st, float2 dst0, float2 dst1)
        {
            Assert_(texture->type == TextureResourceData::Float);

            float result;
            EWA(texture, st, dst0, dst1, result);
            return result;
        }

        //==============================================================================
        float3 PointFloat3(TextureResourceData* texture, float2 st)
        {
            Assert_(texture->type == TextureResourceData::Float3);

            float3 result;
            Point(texture, st, result);
            return result;
        }

        //==============================================================================
        float3 TriangleFloat3(TextureResourceData* texture, int32 level, float2 st)
        {
            Assert_(texture->type == TextureResourceData::Float3);

            float3 result;
            Triangle(texture, level, st, result);
            return result;
        }

        //==============================================================================
        float3 EWAFloat3(TextureResourceData* texture, float2 st, float2 dst0, float2 dst1)
        {
            Assert_(texture->type == TextureResourceData::Float3);

            float3 result;
            EWA(texture, st, dst0, dst1, result);
            return result;
        }
    }
}