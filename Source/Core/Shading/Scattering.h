#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "MathLib/FloatStructs.h"

namespace Selas
{
    //=============================================================================================================================
    enum SurfaceEventTypes
    {
        eScatterEvent,
        eTransmissionEvent
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
        SurfaceEventTypes type;

        MediumParameters medium = MediumParameters();
        float3 reflectance      = float3::Zero_;
        float3 wi               = float3::Zero_;
        float forwardPdfW       = 0.0f;
        float reversePdfW       = 0.0f;
    };
}