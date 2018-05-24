//==============================================================================
// Joe Schutte
//==============================================================================

#include <GeometryLib/Disc.h>
#include <MathLib/Trigonometric.h>

namespace Shooty
{
    //==============================================================================
    // -- "From A Low Distortion Map Between Disk and Square" by Peter Shirley and Kenneth Chiu
    // -- Copied from https://github.com/TheRealMJP/BakingLab because typing is for suckers.
    float2 SampleConcentricDisc(float r0, float r1)
    {
        float phi, r;

        // -- (a,b) is now on [-1,1]ˆ2
        float a = 2.0f * r0 - 1.0f;
        float b = 2.0f * r1 - 1.0f;

        if(a > -b)                      // region 1 or 2
        {
            if(a > b)                   // region 1, also |a| > |b|
            {
                r = a;
                phi = (Math::Pi_ / 4.0f) * (b / a);
            }
            else                        // region 2, also |b| > |a|
            {
                r = b;
                phi = (Math::Pi_ / 4.0f) * (2.0f - (a / b));
            }
        }
        else                            // region 3 or 4
        {
            if(a < b)                   // region 3, also |a| >= |b|, a != 0
            {
                r = -a;
                phi = (Math::Pi_ / 4.0f) * (4.0f + (b / a));
            }
            else                        // region 4, |b| >= |a|, but a==0 and b==0 could occur.
            {
                r = -b;
                if(Math::Absf(b) > 0.0f)
                    phi = (Math::Pi_ / 4.0f) * (6.0f - (a / b));
                else
                    phi = 0;
            }
        }

        float2 result;
        result.x = r * Math::Cosf(phi);
        result.y = r * Math::Sinf(phi);

        return result;
    }

    //==============================================================================
    float ConcentricDiscPdf()
    {
        return Math::OOPi_;
    }
}