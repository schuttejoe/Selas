#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "Shading/IntegratorContexts.h"
#include "GeometryLib/Ray.h"
#include "ContainersLib/CArray.h"
#include "MathLib/FloatStructs.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    struct DeferredBatch;
    struct OcclusionBatch;
    struct HitBatch;

    // JSTODO - Octrahedral format for direction
    // JSTODO - RGB9e5 or half format for throughput
    struct DeferredRay
    {
        Ray ray;
        float3 throughput; 
        
        uint32 index : 30;
        uint32 diracScatterOnly : 31;

        float  error;
    };

    struct OcclusionRay
    {
        Ray ray;
        float distance;
        float3 value;
        uint32 index;
    };

    enum RayBatchCategory
    {
        PositiveX,
        NegativeX,
        PositiveY,
        NegativeY,
        PositiveZ,
        NegativeZ,

        RayBatchCategoryCount
    };

    class PathTracingBatcher
    {
    private:
        void* lock;
        int64 batchIndex;
        Align_(64) DeferredBatch* currentDeferred[RayBatchCategoryCount];
        Align_(64) OcclusionBatch* currentOcclusion[RayBatchCategoryCount];
        Align_(64) HitBatch* currentHits;

        CArray<DeferredBatch*>  deferredBatches;
        CArray<DeferredBatch*>  readyDeferredBatches;
        CArray<OcclusionBatch*> occlusionBatches;
        CArray<OcclusionBatch*> readyOcclusionBatches;
        CArray<HitBatch*>       hitBatches;
        CArray<HitBatch*>       readyHitBatches;

        int64 rayBatchCapacity;
        int64 hitBatchCapacity;
        
        uint64 totalEntriesAdded;
        uint64 totalEntriesConsumed;

        DeferredBatch* AllocateRayBatch(RayBatchCategory category);
        void FlushCompletedBatch(DeferredBatch* batch);
        void LoadBatch(DeferredBatch* batch);

        OcclusionBatch* AllocateOcclusionBatch(RayBatchCategory category);
        void FlushCompletedBatch(OcclusionBatch* batch);
        void LoadBatch(OcclusionBatch* batch);

        HitBatch* AllocateHitBatch();
        void FlushCompletedBatch(HitBatch* batch);
        void LoadBatch(HitBatch* batch);

    public:

        PathTracingBatcher();
        ~PathTracingBatcher();

        void Initialize(uint rayBatchCapacity, uint hitBatchCapacity);
        void Shutdown();

        void AddUnsortedDeferredRay(const DeferredRay& ray);
        void AddUnsortedOcclusionRay(const OcclusionRay& ray);
        void AddUnsortedHit(const HitParameters& hit);

        void Flush();

        bool GetSortedBatch(DeferredRay*& rays, uint& rayCount);
        void FreeRays(DeferredRay* rays);

        bool GetSortedBatch(OcclusionRay*& rays, uint& rayCount);
        void FreeRays(OcclusionRay* rays);

        bool GetSortedHits(HitParameters*& rays, uint& hitCount);
        void FreeHits(HitParameters* hits);

        bool Empty();
    };
}