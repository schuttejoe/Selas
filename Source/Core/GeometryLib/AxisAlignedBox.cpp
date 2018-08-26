//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "GeometryLib/AxisAlignedBox.h"
#include "IoLib/Serializer.h"
#include "MathLib/FloatFuncs.h"
#include "SystemLib/MinMax.h"

namespace Selas
{
    //=============================================================================================================================
    void Serialize(CSerializer* serializer, AxisAlignedBox& v)
    {
        Serialize(serializer, v.min);
        Serialize(serializer, v.max);
    }

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

    //=============================================================================================================================
    void IncludeBox(AxisAlignedBox* box, float4x4 transform, const AxisAlignedBox& right)
    {
        float3 d = right.max - right.min;
        float3 ex = float3(d.x, 0.0f, 0.0f);
        float3 ey = float3(0.0f, d.y, 0.0f);
        float3 ez = float3(0.0f, 0.0f, d.z);

        float3 tmin = MatrixMultiplyPoint(right.min, transform);
        ex = MatrixMultiplyVector(ex, transform);
        ey = MatrixMultiplyVector(ey, transform);
        ez = MatrixMultiplyVector(ez, transform);

        IncludePosition(box, tmin);
        IncludePosition(box, tmin + ex);
        IncludePosition(box, tmin + ex + ez);
        IncludePosition(box, tmin + ez);

        IncludePosition(box, tmin + ey);
        IncludePosition(box, tmin + ey + ex);
        IncludePosition(box, tmin + ey + ez);
        IncludePosition(box, tmin + ey + ex + ez);
    }

    //=============================================================================================================================
    void IncludeBox(AxisAlignedBox* box, const AxisAlignedBox& right)
    {
        IncludePosition(box, right.min);
        IncludePosition(box, right.max);
    }
}