#pragma once

//==============================================================================
// (c)2012 Joe Schutte
//==============================================================================
#include "FloatStructs.h"

namespace Shooty {

    namespace Math {

        // Reference:
        // "An Efficient Representation for Irradiance Environment Maps" [Ramamoorthi]
        // http://cseweb.ucsd.edu/~ravir/papers/envmap/envmap.pdf

        // "Stupid spherical harmonics (sh) tricks" [Sloan]
        // http://www.ppsloan.org/publications/StupidSH36.pdf

        struct SphericalHarmonic9 {
            SphericalHarmonic9() {}
            float coefficients[9];
        };

        struct SphericalHarmonic9Color {
            SphericalHarmonic9 red;
            SphericalHarmonic9 green;
            SphericalHarmonic9 blue;
        };

        namespace SH9 {
            void                Zero(SphericalHarmonic9* sh);
            SphericalHarmonic9  Add(const SphericalHarmonic9& left, const SphericalHarmonic9& right);
            SphericalHarmonic9  Subtract(const SphericalHarmonic9& left, const SphericalHarmonic9& right);
            SphericalHarmonic9  Scale(const SphericalHarmonic9& left, float scale);
            float               Dot(const SphericalHarmonic9& left, const SphericalHarmonic9& right);
            SphericalHarmonic9  EvaluateBasis(const float3& v);
            SphericalHarmonic9  Project(const float3& v, float intensity);

            void                     Zero(SphericalHarmonic9Color* sh);
            SphericalHarmonic9Color  Add(const SphericalHarmonic9Color& left, const SphericalHarmonic9Color& right);
            SphericalHarmonic9Color  Subtract(const SphericalHarmonic9Color& left, const SphericalHarmonic9Color& right);
            SphericalHarmonic9Color  Scale(const SphericalHarmonic9Color& left, float scale);
            float3                   Dot(const SphericalHarmonic9Color& left, const SphericalHarmonic9Color& right);
            SphericalHarmonic9Color  Project(const float3& v, const float3& intensity);

        }
    }
}