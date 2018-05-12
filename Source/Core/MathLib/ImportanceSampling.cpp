//==============================================================================
// Joe Schutte
//==============================================================================

#include "ImportanceSampling.h"
#include <MathLib/Trigonometric.h>
#include <MathLib/FloatStructs.h>
#include <MathLib/FloatFuncs.h>
#include <MathLib/Projection.h>
#include <SystemLib/MemoryAllocation.h>
#include <SystemLib/JsAssert.h>
#include <SystemLib/MinMax.h>

namespace Shooty
{
    namespace ImportanceSampling
    {
        //==============================================================================
        float BalanceHeuristic(uint nf, float fPdf, uint ng, float gPdf)
        {
            return (nf * fPdf) / (nf * fPdf + ng * gPdf);
        }

        //==============================================================================
        float PowerHeuristic(uint nf, float fPdf, uint ng, float gPdf)
        {
            float f = nf * fPdf;
            float g = ng * gPdf;
            return (f * f) / (f * f + g * g);
        }
    }
}