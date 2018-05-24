//==============================================================================
// Joe Schutte
//==============================================================================

#include <GeometryLib/RectangulerLightSampler.h>
#include <MathLib/FloatFuncs.h>
#include <MathLib/Trigonometric.h>
#include <SystemLib/MinMax.h>

// -- Taken from:
// https://www.solidangle.com/research/egsr2013_spherical_rectangle.pdf
// -- See also: :P
// https://schuttejoe.github.io/post/arealightsampling/

namespace Selas
{
    void InitializeRectangleLightSampler(float3 s, float3 eX, float3 eY, float3 o, RectangleLightSampler& sampler)
    {
        sampler.o = o;

        float eXLength = Length(eX);
        float eYLength = Length(eY);

        // compute local reference system ’R’
        sampler.x = eX * (1.0f / eXLength);
        sampler.y = eY * (1.0f / eYLength);
        sampler.z = Cross(sampler.x, sampler.y);

        // compute rectangle coordinates in local reference system
        float3 d = s - o;
        sampler.z0 = Dot(d, sampler.z);

        // flip ’z’ to make it point against ’Q’
        if(sampler.z0 > 0) {
            sampler.z = -sampler.z;
            sampler.z0 *= -1;
        }

        sampler.z0sq = sampler.z0 * sampler.z0;
        sampler.x0 = Dot(d, sampler.x);
        sampler.y0 = Dot(d, sampler.y);
        sampler.x1 = sampler.x0 + eXLength;
        sampler.y1 = sampler.y0 + eYLength;
        sampler.y0sq = sampler.y0 * sampler.y0;
        sampler.y1sq = sampler.y1 * sampler.y1;

        // create vectors to four vertices
        float3 v00 = float3(sampler.x0, sampler.y0, sampler.z0);
        float3 v01 = float3(sampler.x0, sampler.y1, sampler.z0);
        float3 v10 = float3(sampler.x1, sampler.y0, sampler.z0);
        float3 v11 = float3(sampler.x1, sampler.y1, sampler.z0);

        // compute normals to edges
        float3 n0 = Normalize(Cross(v00, v10));
        float3 n1 = Normalize(Cross(v10, v11));
        float3 n2 = Normalize(Cross(v11, v01));
        float3 n3 = Normalize(Cross(v01, v00));

        // compute internal angles (gamma_i)
        float g0 = Math::Acosf(-Dot(n0, n1));
        float g1 = Math::Acosf(-Dot(n1, n2));
        float g2 = Math::Acosf(-Dot(n2, n3));
        float g3 = Math::Acosf(-Dot(n3, n0));

        // compute predefined constants
        sampler.b0 = n0.z;
        sampler.b1 = n2.z;
        sampler.b0sq = sampler.b0 * sampler.b0;
        sampler.k = 2 * Math::Pi_ - g2 - g3;

        // compute solid angle from internal angles
        sampler.S = g0 + g1 - sampler.k;
    }

    float3 SampleRectangleLight(const RectangleLightSampler& sampler, float u, float v)
    {
        // 1. compute cu
        float au = u * sampler.S + sampler.k;
        float fu = (Math::Cosf(au) * sampler.b0 - sampler.b1) / Math::Sinf(au);
        float cu = 1 / Math::Sqrtf(fu*fu + sampler.b0sq) * (fu > 0 ? +1 : -1);

        cu = Clamp<float>(cu, -1.0f, 1.0f); // avoid NaNs

        // 2. compute xu
        float xu = -(cu * sampler.z0) / Math::Sqrtf(1 - cu * cu);
        xu = Clamp<float>(xu, sampler.x0, sampler.x1); // avoid Infinity

        // 3. compute yv
        float d = Math::Sqrtf(xu * xu + sampler.z0sq);
        float h0 = sampler.y0 / Math::Sqrtf(d * d + sampler.y0sq);
        float h1 = sampler.y1 / Math::Sqrtf(d * d + sampler.y1sq);
        float hv = h0 + v * (h1 - h0), hv2 = hv * hv;
        float yv = (hv2 < 1 - SmallFloatEpsilon_) ? (hv * d) / Math::Sqrtf(1 - hv2) : sampler.y1;

        // 4. transform (xu, yv, z0) to world coordinates
        return (sampler.o + xu * sampler.x + yv * sampler.y + sampler.z0 * sampler.z);
    }
}