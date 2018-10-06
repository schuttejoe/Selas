#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "MathLib/FloatStructs.h"

namespace Selas
{
    //=============================================================================================================================
    enum SurfaceEventFlags
    {
        eScatterEvent        = 0x01,
        eTransmissionEvent   = 0x02,
        eDiracEvent          = 0x04 
    };

    //=============================================================================================================================
    enum MediumPhaseFunction
    {
        eVacuum,
        eIsotropic
    };

    //=============================================================================================================================
    struct MediumParameters
    {
        MediumPhaseFunction phaseFunction = eVacuum;
        float3 extinction = float3::Zero_;
    };

    //=============================================================================================================================
    struct BsdfSample
    {
        uint32 flags;

        MediumParameters medium = MediumParameters();
        float3 reflectance      = float3::Zero_;
        float3 wi               = float3::Zero_;
        float forwardPdfW       = 0.0f;
        float reversePdfW       = 0.0f;
    };
}