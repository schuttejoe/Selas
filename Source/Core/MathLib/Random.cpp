//==============================================================================
// Joe Schutte
//==============================================================================

#include <MathLib\Random.h>
#include <MathLib\FloatStructs.h>
#include <MathLib\FloatFuncs.h>
#include <MathLib\Quaternion.h>
#include <MathLib\Trigonometric.h>
#include <SystemLib\JsAssert.h>
#include <stdlib.h>

namespace Shooty {
    namespace Random {

        // -- JSTOTO - Use std's merseinne twister

        //==============================================================================
        uint RandUint(uint max) {
            return rand() % max;
        }

        //==============================================================================
        float RandFloat0_1(void) {
            float random = (float)rand() / (float)RAND_MAX;
            Assert_(random >= 0.0f && random <= 1.0f);
            return random;
        }

        //==============================================================================
        void FillRandomBuffer(float* buffer, uint count, uint32 seed) {
            srand(seed);

            for (uint scan = 0; scan < count; ++scan) {
                buffer[scan] = RandFloat0_1();
            }
        }

        //==============================================================================
        void FillRandomBuffer(float4* buffer, uint count, uint32 seed) {
            srand(seed);

            for (uint scan = 0; scan < count; ++scan) {
                buffer[scan].x = RandFloat0_1();
                buffer[scan].y = RandFloat0_1();
                buffer[scan].z = RandFloat0_1();
                buffer[scan].w = RandFloat0_1();
            }
        }

        //==============================================================================
        float3 UniformConeRandom(float3& direction, float maxTheta) {
            Assert_(maxTheta <= 3.14159265f);
            float scale = maxTheta * 1.0f / 3.1415926535f;

            float theta = scale * Math::Acosf(2.0f * RandFloat0_1() - 1.0f);
            float phi = RandFloat0_1() * 6.2831853f;

            float3 local;
            local.x = Math::Sinf(theta) * Math::Cosf(phi);
            local.y = Math::Cosf(theta);
            local.z = Math::Sinf(theta) * Math::Sinf(phi);

            float3 yUp = float3(0.0f, 1.0f, 0.0f);
            float3 axis = Cross(yUp, direction);
            float angle = Math::Acosf(Dot(direction, yUp));

            float4 quat = Math::Quaternion::Create(angle, axis);
            return Normalize(Math::Quaternion::Rotate(quat, local));
        }
    }
}