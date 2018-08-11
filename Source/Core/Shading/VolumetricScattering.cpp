
//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "Shading/VolumetricScattering.h"
#include "Shading/SurfaceParameters.h"
#include "Shading/IsotropicScattering.h"
#include "SystemLib/JsAssert.h"

namespace Selas
{
    //=============================================================================================================================
    float SampleDistance(CSampler* sampler, const MediumParameters& medium, float* pdf)
    {
        if(medium.phaseFunction == eVacuum) {
            return FloatMax_;
        }
        else if(medium.phaseFunction == eIsotropic) {
            return Isotropic::SampleDistance(sampler, medium, pdf);
        }

        return 1.0f;
    }

    //=============================================================================================================================
    float3 SampleScatterDirection(CSampler* sampler, const MediumParameters& medium, float3 wo, float* pdf)
    {
        if(medium.phaseFunction == eVacuum) {
            return wo;
        }
        else if(medium.phaseFunction == eIsotropic) {
            return Isotropic::SampleScatterDirection(sampler, medium, wo, pdf);
        }

        return float3::YAxis_;
    }

    //=============================================================================================================================
    float ScatterDirectionPdf(const MediumParameters& medium, float3 wo, float3 wi)
    {
        if(medium.phaseFunction == eVacuum) {
            return 1.0f;
        }
        else if(medium.phaseFunction == eIsotropic) {
            return Isotropic::ScatterDirectionPdf(medium, wo, wi);
        }

        return 1.0f;
    }

    //=============================================================================================================================
    float3 Transmission(const MediumParameters& medium, float distance)
    {
        if(medium.phaseFunction == eVacuum) {
            return float3::One_;
        }
        else if(medium.phaseFunction == eIsotropic) {
            return Isotropic::Transmission(medium, distance);
        }

        return float3::Zero_;
    }
}