//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "MathLib/SphericalHarmonic.h"
#include "SystemLib/Memory.h"

namespace Selas
{
    namespace Math
    {
        namespace SH9
        {
            //=====================================================================================================================
            void Zero(SphericalHarmonic9* sh)
            {
                Memory::Set(sh->coefficients, 0, sizeof(sh->coefficients));
            }

            //=====================================================================================================================
            SphericalHarmonic9 Add(const SphericalHarmonic9& left, const SphericalHarmonic9& right)
            {
                SphericalHarmonic9 res;
                for(uint scan = 0; scan < 9; ++scan) {
                    res.coefficients[scan] = left.coefficients[scan] + right.coefficients[scan];
                }

                return res;
            }

            //=====================================================================================================================
            SphericalHarmonic9 Subtract(const SphericalHarmonic9& left, const SphericalHarmonic9& right)
            {
                SphericalHarmonic9 res;
                for(uint scan = 0; scan < 9; ++scan) {
                    res.coefficients[scan] = left.coefficients[scan] - right.coefficients[scan];
                }

                return res;
            }

            //=====================================================================================================================
            SphericalHarmonic9 Scale(const SphericalHarmonic9& left, float scale)
            {
                SphericalHarmonic9 res;
                for(uint scan = 0; scan < 9; ++scan) {
                    res.coefficients[scan] = left.coefficients[scan] * scale;
                }

                return res;
            }

            //=====================================================================================================================
            float Dot(const SphericalHarmonic9& left, const SphericalHarmonic9& right)
            {
                float sum = 0.0f;
                for(uint scan = 0; scan < 9; ++scan) {
                    sum += left.coefficients[scan] * right.coefficients[scan];
                }

                return sum;
            }

            //=====================================================================================================================
            SphericalHarmonic9 EvaluateBasis(const float3& v)
            {
                SphericalHarmonic9 res;

                // -- band 0
                res.coefficients[0] = 0.282095f;

                // -- band 1
                res.coefficients[1] = 0.488603f * v.y;
                res.coefficients[2] = 0.488603f * v.z;
                res.coefficients[3] = 0.488603f * v.x;

                float xy = v.x * v.y;
                float yz = v.y * v.z;
                float xz = v.x * v.z;
                float xx = v.x * v.x;
                float yy = v.y * v.y;
                float zz = v.z * v.z;

                // -- band 2
                res.coefficients[4] = 1.092548f * xy;
                res.coefficients[5] = 1.092548f * yz;
                res.coefficients[6] = 0.315392f * (3.0f * zz - 1.0f);
                res.coefficients[7] = 1.092548f * xz;
                res.coefficients[8] = 0.546274f * (xx - yy);

                return res;
            }

            //=====================================================================================================================
            SphericalHarmonic9 Project(const float3& v, float intensity)
            {
                SphericalHarmonic9 res = EvaluateBasis(v);
                for(uint scan = 0; scan < 9; ++scan) {
                    res.coefficients[scan] = res.coefficients[scan] * intensity;
                }

                return res;
            }

            //=====================================================================================================================
            void Zero(SphericalHarmonic9Color* sh)
            {
                Zero(&sh->red);
                Zero(&sh->green);
                Zero(&sh->blue);
            }

            //=====================================================================================================================
            SphericalHarmonic9Color Add(const SphericalHarmonic9Color& left, const SphericalHarmonic9Color& right)
            {
                SphericalHarmonic9Color color;
                color.red = Add(left.red, right.red);
                color.green = Add(left.green, right.green);
                color.blue = Add(left.blue, right.blue);

                return color;
            }

            //=====================================================================================================================
            SphericalHarmonic9Color Subtract(const SphericalHarmonic9Color& left, const SphericalHarmonic9Color& right)
            {
                SphericalHarmonic9Color color;
                color.red = Subtract(left.red, right.red);
                color.green = Subtract(left.green, right.green);
                color.blue = Subtract(left.blue, right.blue);

                return color;
            }

            //=====================================================================================================================
            SphericalHarmonic9Color Scale(const SphericalHarmonic9Color& left, float scale)
            {
                SphericalHarmonic9Color color;
                color.red = Scale(left.red, scale);
                color.green = Scale(left.green, scale);
                color.blue = Scale(left.blue, scale);

                return color;
            }

            //=====================================================================================================================
            float3 Dot(const SphericalHarmonic9Color& left, const SphericalHarmonic9Color& right)
            {
                float3 sum;
                sum.x = Dot(left.red, right.red);
                sum.y = Dot(left.green, right.green);
                sum.z = Dot(left.blue, right.blue);

                return sum;
            }

            //=====================================================================================================================
            SphericalHarmonic9Color Project(const float3& v, const float3& intensity)
            {
                SphericalHarmonic9 sh = EvaluateBasis(v);

                SphericalHarmonic9Color res;
                Zero(&res);
                for(uint scan = 0; scan < 9; ++scan) {
                    res.red.coefficients[scan] = sh.coefficients[scan] * intensity.x;
                    res.green.coefficients[scan] = sh.coefficients[scan] * intensity.y;
                    res.blue.coefficients[scan] = sh.coefficients[scan] * intensity.z;
                }

                return res;
            }
        }
    }
}