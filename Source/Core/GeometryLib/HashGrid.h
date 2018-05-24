#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <GeometryLib/AxisAlignedBox.h>
#include <MathLib/FloatStructs.h>
#include <ContainersLib/CArray.h>
#include <SystemLib/BasicTypes.h>

namespace Selas
{
    struct HashGrid
    {
        CArray<uint32> cellIndices;
        CArray<uint32> cellRangeEnds;
        AxisAlignedBox aaBox;

        uint  cellCount;
        float radius;
        float radiusSquare;
        float inverseCellSize;
    };

    typedef void(*HashGridCallbackFunction)(uint index, void* userData);

    void BuildHashGrid(HashGrid* hashGrid, uint cellCount, float radius, const CArray<float3>& points);
    void ShutdownHashGrid(HashGrid* hashGrid);

    void SearchHashGrid(HashGrid* hashGrid, const CArray<float3>& points, float3 position, void* userData, HashGridCallbackFunction callback);
}