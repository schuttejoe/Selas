//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "GeometryLib/AxisAlignedBox.h"
#include "MathLib/FloatFuncs.h"
#include "SystemLib/MinMax.h"

namespace Selas
{
    //=============================================================================================================================
    void MakeInvalid(AxisAlignedBox* box)
    {
        box->min.x = FloatMax_;
        box->min.y = FloatMax_;
        box->min.z = FloatMax_;

        box->max.x = -FloatMax_;
        box->max.y = -FloatMax_;
        box->max.z = -FloatMax_;
    }

    //=============================================================================================================================
    void IncludePosition(AxisAlignedBox* box, float3 position)
    {
        box->min.x = Min<float>(box->min.x, position.x);
        box->min.y = Min<float>(box->min.y, position.y);
        box->min.z = Min<float>(box->min.z, position.z);

        box->max.x = Max<float>(box->max.x, position.x);
        box->max.y = Max<float>(box->max.y, position.y);
        box->max.z = Max<float>(box->max.z, position.z);
    }
}