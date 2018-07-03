//==============================================================================
// Joe Schutte
//==============================================================================

#include "VCMHashGrid.h"
#include "VCMCommon.h"
#include "MathLib/IntStructs.h"
#include "MathLib/FloatFuncs.h"
#include "MathLib/Trigonometric.h"
#include "SystemLib/Memory.h"

namespace Selas
{
    //==============================================================================
    static uint CalculateCellIndex(int3 xyz, uint cellCount)
    {
        uint x = uint(xyz.x);
        uint y = uint(xyz.y);
        uint z = uint(xyz.z);

        return int(((x * 73856093) ^ (y * 19349663) ^ (z * 83492791)) % cellCount);
    }

    //==============================================================================
    static uint CalculateCellIndex(VCMHashGrid* __restrict hashGrid, float3 point)
    {
        float3 fromCorner = point - hashGrid->aaBox.min;

        int3 xyz;
        xyz.x = (int32)Math::Floor(hashGrid->inverseCellSize * fromCorner.x);
        xyz.y = (int32)Math::Floor(hashGrid->inverseCellSize * fromCorner.y);
        xyz.z = (int32)Math::Floor(hashGrid->inverseCellSize * fromCorner.z);

        return CalculateCellIndex(xyz, hashGrid->cellCount);
    }

    //==============================================================================
    static int2 GetCellRange(VCMHashGrid* __restrict hashGrid, uint cellIndex)
    {
        if(cellIndex == 0) {
            return int2(0, hashGrid->cellRangeEnds[0]);
        }

        return int2(hashGrid->cellRangeEnds[cellIndex - 1], hashGrid->cellRangeEnds[cellIndex]);
    }

    //==============================================================================
    void BuildHashGrid(VCMHashGrid* __restrict hashGrid, uint cellCount, float radius, const CArray<VCMVertex>& points)
    {
        float radiusSquare    = radius * radius;
        float cellSize        = 2.0f * radius;
        float inverseCellSize = 1.0f / cellSize;

        hashGrid->radius          = radius;
        hashGrid->radiusSquare    = radiusSquare;
        hashGrid->inverseCellSize = inverseCellSize;
        hashGrid->cellCount       = cellCount;

        uint pointCount = points.Length();

        hashGrid->cellRangeEnds.Resize((uint32)cellCount);
        hashGrid->cellIndices.Resize((uint32)pointCount);
        Memory::Zero(hashGrid->cellRangeEnds.GetData(), hashGrid->cellRangeEnds.DataSize());

        // -- Prep the AABox
        MakeInvalid(&hashGrid->aaBox);
        for(uint scan = 0; scan < pointCount; ++scan) {
            IncludePosition(&hashGrid->aaBox, points[scan].hit.position);
        }
        // JSTODO - Verify this. Seems like a bug in the original author's implementation to not do this.
        //hashGrid->aaBox.min = hashGrid->aaBox.min - radius;
        //hashGrid->aaBox.max = hashGrid->aaBox.max + radius;

        // -- Count the number of particles that will be in each cell
        for(uint scan = 0; scan < pointCount; ++scan) {
            uint cellIndex = CalculateCellIndex(hashGrid, points[scan].hit.position);
            ++hashGrid->cellRangeEnds[cellIndex];
        }

        // -- prefix sum of the contents of cellRangeEnds (which is currently the count of particles per cell).
        // -- This sets each cellRangeEnds[x] to the cell's start index.
        uint32 sum = 0;
        for(uint scan = 0; scan < cellCount; ++scan) {
            uint32 rangeCount = hashGrid->cellRangeEnds[scan];
            hashGrid->cellRangeEnds[scan] = sum;
            sum += rangeCount;
        }

        // -- Assign each point to an index. This sums up each cell such that each cellRangeEnds[x] will now be the exclusive end index of that cell's range
        for(uint scan = 0; scan < pointCount; ++scan) {
            uint cellIndex = CalculateCellIndex(hashGrid, points[scan].hit.position);

            uint32 targetIndex = hashGrid->cellRangeEnds[cellIndex]++;
            hashGrid->cellIndices[targetIndex] = (uint32)scan;
        }
    }

    //==============================================================================
    void ShutdownHashGrid(VCMHashGrid* hashGrid)
    {
        hashGrid->cellIndices.Close();
        hashGrid->cellRangeEnds.Close();
    }

    //==============================================================================
    void SearchHashGrid(VCMHashGrid* __restrict hashGrid, const CArray<VCMVertex>& vertices, float3 position, void* userData, HashGridCallbackFunction callback)
    {
        // -- Verify the given position is within the range
        float3 deltaMin = position - hashGrid->aaBox.min;
        float3 deltaMax = hashGrid->aaBox.max - position;
        if(deltaMin.x < 0.0f || deltaMin.y < 0.0f || deltaMin.z < 0.0f) return;
        if(deltaMax.x < 0.0f || deltaMax.y < 0.0f || deltaMax.z < 0.0f) return;

        // -- calculate xyz coordinate start indices
        float3 cellPoint = hashGrid->inverseCellSize * deltaMin;
        float3 coordsF;
        coordsF.x = Math::Floor(cellPoint.x);
        coordsF.y = Math::Floor(cellPoint.y);
        coordsF.z = Math::Floor(cellPoint.z);

        float3 fractional = cellPoint - coordsF;

        int3 xyzMin;
        xyzMin.x = (int32)coordsF.x;
        xyzMin.y = (int32)coordsF.y;
        xyzMin.z = (int32)coordsF.z;

        if(fractional.x < 0.5f)
            --xyzMin.x;
        if(fractional.y < 0.5f)
            --xyzMin.y;
        if(fractional.z < 0.5f)
            --xyzMin.z;

        for(int32 z = 0; z <= 1; ++z) {
            for(int32 y = 0; y <= 1; ++y) {
                for(int32 x = 0; x <= 1; ++x) {
                    uint cellIndex = CalculateCellIndex(int3(xyzMin.x + x, xyzMin.y + y, xyzMin.z + z), hashGrid->cellCount);
                    int2 cellRange = GetCellRange(hashGrid, cellIndex);

                    for(uint scan = cellRange.x; scan < cellRange.y; ++scan) {
                        uint particleIndex = hashGrid->cellIndices[scan];
                        const VCMVertex& vertex = vertices[particleIndex];

                        float distSquared = LengthSquared(position - vertex.hit.position);
                        if(distSquared <= hashGrid->radiusSquare) {
                            callback(vertex, userData);
                        }
                    }
                }
            }
        }
    }
}