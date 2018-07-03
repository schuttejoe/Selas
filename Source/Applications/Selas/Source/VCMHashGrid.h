#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "GeometryLib/AxisAlignedBox.h"
#include "MathLib/FloatStructs.h"
#include "ContainersLib/CArray.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    struct VCMVertex;

    struct VCMHashGrid
    {
        CArray<uint32> cellIndices;
        CArray<uint32> cellRangeEnds;
        AxisAlignedBox aaBox;

        uint  cellCount;
        float radius;
        float radiusSquare;
        float inverseCellSize;
    };

    typedef void(*HashGridCallbackFunction)(const VCMVertex& vertex, void* userData);

    void BuildHashGrid(VCMHashGrid* hashGrid, uint cellCount, float radius, const CArray<VCMVertex>& points);
    void ShutdownHashGrid(VCMHashGrid* hashGrid);

    void SearchHashGrid(const VCMHashGrid* hashGrid, const CArray<VCMVertex>& vertices, float3 position, void* userData, HashGridCallbackFunction callback);
}