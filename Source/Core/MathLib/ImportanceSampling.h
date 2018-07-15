#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

//#include "MathLib/FloatStructs.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    struct float3;

    namespace ImportanceSampling
    {
        float BalanceHeuristic(uint nf, float fPdf, uint ng, float gPdf);
        float PowerHeuristic(uint nf, float fPdf, uint ng, float gPdf);
    }
}
