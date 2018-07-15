//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "GeometryLib/Ray.h"
#include "GeometryLib/SurfaceDifferentials.h"
#include "MathLib/FloatFuncs.h"

namespace Selas
{
    //=============================================================================================================================
    Ray MakeRay(float3 origin, float3 direction)
    {
        Ray ray;
        ray.origin      = origin;
        ray.direction   = direction;

        return ray;
    }
}