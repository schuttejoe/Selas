
//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "Shading/IsotropicScattering.h"
#include "Shading/Scattering.h"
#include "Shading/SurfaceParameters.h"
#include "MathLib/Sampler.h"
#include "MathLib/Trigonometric.h"
#include "MathLib/Projection.h"
#include "MathLib/FloatFuncs.h"
#include "SystemLib/JsAssert.h"

namespace Selas
{
    namespace Isotropic
    {
        //=========================================================================================================================
        float SampleDistance(CSampler* sampler, const MediumParameters& medium, float* pdf)
        {
            float ps = Dot(medium.extinction, float3::One_);

            float pr = medium.extinction.x / ps;
            float pg = medium.extinction.y / ps;
            float pb = medium.extinction.z / ps;

            float c;
            float p;

            float r0 = sampler->UniformFloat();
            if(r0 < pr) {
                c = medium.extinction.x;
                p = pr;
            }
            else if(r0 < pr + pg) {
                c = medium.extinction.y;
                p = pg;
            }
            else {
                c = medium.extinction.z;
                p = pb;
            }

            float s = -Math::Ln(sampler->UniformFloat()) / c;
            *pdf = Math::Expf(-c * s) / p;

            return s;
        }

        //=========================================================================================================================
        float3 SampleScatterDirection(CSampler* sampler, const MediumParameters& medium, float3 wo, float* pdf)
        {
            return sampler->UniformSphere();
        }

        //=========================================================================================================================
        float ScatterDirectionPdf(const MediumParameters& medium, float3 wo, float3 wi)
        {
            return CSampler::UniformSpherePdf();
        }

        //=========================================================================================================================
        float3 Transmission(const MediumParameters& medium, float distance)
        {
            float tr = Math::Expf(-medium.extinction.x * distance);
            float tg = Math::Expf(-medium.extinction.y * distance);
            float tb = Math::Expf(-medium.extinction.z * distance);

            return float3(tr, tg, tb);
        }
    }
}