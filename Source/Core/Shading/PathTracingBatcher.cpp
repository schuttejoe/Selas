
//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "Shading/PathTracingBatcher.h"
#include "UtilityLib/QuickSort.h"
#include "StringLib/FixedString.h"
#include "StringLib/StringUtil.h"
#include "MathLib/Trigonometric.h"
#include "MathLib/FloatFuncs.h"
#include "IoLib/Directory.h"
#include "IoLib/Environment.h"
#include "SystemLib/Atomic.h"
#include "SystemLib/OSThreading.h"
#include "SystemLib/MinMax.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace Selas
{
    // -- Hmm... maybe just add array operators for float2/float3/float4?
    struct DeferredRaySortX : public DeferredRay
    {
        float Position() const { return ray.origin.x; }
        float Direction() const { return ray.direction.x; }
    };
    struct DeferredRaySortY : public DeferredRay
    {
        float Position() const { return ray.origin.y; }
        float Direction() const { return ray.direction.y; }
    };
    struct DeferredRaySortZ : public DeferredRay
    {
        float Position() const { return ray.origin.z; }
        float Direction() const { return ray.direction.z; }
    };
    struct OcclusionRaySortX : public OcclusionRay
    {
        float Position() const { return ray.origin.x; }
        float Direction() const { return ray.direction.x; }
    };
    struct OcclusionRaySortY : public OcclusionRay
    {
        float Position() const { return ray.origin.y; }
        float Direction() const { return ray.direction.y; }
    };
    struct OcclusionRaySortZ : public OcclusionRay
    {
        float Position() const { return ray.origin.z; }
        float Direction() const { return ray.direction.z; }
    };

    //=================================================================================================================================
    static FilePathString CreateBatchFilePath(int64 index)
    {
        FixedString128 root = Environment_Root();

        char ps = StringUtil::PathSeperator();

        FilePathString result;
        FixedStringSprintf(result, "%s_Temp%cbatch_%lld.batch", root.Ascii(), ps, index);

        return result;
    }

    //=================================================================================================================================
    struct DeferredBatch
    {
        DeferredBatch()
            : fileHandle(INVALID_HANDLE_VALUE)
            , mappingHandle(INVALID_HANDLE_VALUE)
        {

        }

        ~DeferredBatch()
        {
            if(fileHandle != INVALID_HANDLE_VALUE) {
                UnmapViewOfFile(rays);
                CloseHandle(fileHandle);
                CloseHandle(mappingHandle);

                FilePathString filepath = CreateBatchFilePath(batchIndex);
                DeleteFileA(filepath.Ascii());
            }
        }

        volatile int64 batchHead;
        volatile int64 batchTail;
        int64 batchIndex;
        RayBatchCategory category;
        HANDLE fileHandle;
        HANDLE mappingHandle;
        DeferredRay* rays;
    };

    //=================================================================================================================================
    struct OcclusionBatch
    {
        OcclusionBatch()
            : fileHandle(INVALID_HANDLE_VALUE)
            , mappingHandle(INVALID_HANDLE_VALUE)
        {

        }

        ~OcclusionBatch()
        {
            if(fileHandle != INVALID_HANDLE_VALUE) {
                UnmapViewOfFile(rays);
                CloseHandle(fileHandle);
                CloseHandle(mappingHandle);

                FilePathString filepath = CreateBatchFilePath(batchIndex);
                DeleteFileA(filepath.Ascii());
            }
        }

        volatile int64 batchHead;
        volatile int64 batchTail;
        int64 batchIndex;
        RayBatchCategory category;
        HANDLE fileHandle;
        HANDLE mappingHandle;
        OcclusionRay* rays;
    };

    struct HitBatch
    {
        HitBatch()
            : fileHandle(INVALID_HANDLE_VALUE)
            , mappingHandle(INVALID_HANDLE_VALUE)
        {

        }

        ~HitBatch()
        {
            if(fileHandle != INVALID_HANDLE_VALUE) {
                UnmapViewOfFile(hits);
                CloseHandle(fileHandle);
                CloseHandle(mappingHandle);

                FilePathString filepath = CreateBatchFilePath(batchIndex);
                DeleteFileA(filepath.Ascii());
            }
        }

        volatile int64 batchHead;
        volatile int64 batchTail;

        int64 batchIndex;
        HANDLE fileHandle;
        HANDLE mappingHandle;
        HitParameters* hits;
    };

    //=================================================================================================================================
    template<typename Type_>
    static RayBatchCategory DetermineRayCategory(const Type_& dray)
    {
        Assert_(LengthSquared(dray.ray.direction) > 0.0f);

        float ax = Math::Absf(dray.ray.direction.x);
        float ay = Math::Absf(dray.ray.direction.y);
        float az = Math::Absf(dray.ray.direction.z);

        if(ax > ay && ax > az) {
            return dray.ray.direction.x > 0.0f ? PositiveX : NegativeX;
        }
        else if(ay > az) {
            return dray.ray.direction.y > 0.0f ? PositiveY : NegativeY;
        }
        else {
            return dray.ray.direction.z > 0.0f ? PositiveZ : NegativeZ;
        }
    }

    //=================================================================================================================================
    template<typename Type_>
    static float Sample(const Type_* ray, uint count)
    {
        return count > 64 ? ray->Position() : ray->Direction();
    }

    //=================================================================================================================================
    template<typename Type_>
    static void SortRaysInternal(Type_* rays, uint count)
    {
        if(count < 2) {
            return;
        }

        Type_* left = rays;
        Type_* right = rays + count - 1;
        float pivot = Sample(&rays[count / 2], count);

        while(left <= right) {
            if(Sample(left, count) < pivot) {
                left++;
            }
            else if(Sample(right, count) > pivot) {
                right--;
            }
            else {
                Type_ temp = *left;
                *left = *right;
                *right = temp;
                left++;
                right--;
            }
        }

        SortRaysInternal(rays, right - rays + 1);
        SortRaysInternal(left, rays + count - left);
    }

    //=================================================================================================================================
    static bool operator<(const HitParameters& lhs, const HitParameters& rhs)
    {
        for(uint scan = 0; scan < MaxInstanceLevelCount_; ++scan) {
            if(lhs.instId[scan] < rhs.instId[scan]) {
                return true;
            }
        }
        
        if(lhs.geomId < rhs.geomId) {
            return true;
        }

        if(lhs.primId < rhs.primId) {
            return true;
        }

        return false;
    }

    //=================================================================================================================================
    static bool operator>(const HitParameters& lhs, const HitParameters& rhs)
    {
        for(uint scan = 0; scan < MaxInstanceLevelCount_; ++scan) {
            if(lhs.instId[scan] > rhs.instId[scan]) {
                return true;
            }
        }

        if(lhs.geomId > rhs.geomId) {
            return true;
        }

        if(lhs.primId > rhs.primId) {
            return true;
        }

        return false;
    }

    //=================================================================================================================================
    DeferredBatch* PathTracingBatcher::AllocateRayBatch(RayBatchCategory category)
    {
        DeferredBatch* batch = New_(DeferredBatch);

        batch->batchIndex = Atomic::Increment64(&batchIndex);
        batch->batchHead = 0;
        batch->batchTail = 0;
        batch->category = category;

        FilePathString path = CreateBatchFilePath(batch->batchIndex);

        Directory::EnsureDirectoryExists(path.Ascii());

        batch->fileHandle = CreateFileA(path.Ascii(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);
        Assert_(batch->fileHandle != INVALID_HANDLE_VALUE);

        Assert_(rayBatchCapacity * sizeof(DeferredRay) < 0xFFFFFFFF);
        DWORD size = (DWORD)(rayBatchCapacity * sizeof(DeferredRay));

        batch->mappingHandle = CreateFileMapping(batch->fileHandle, NULL, PAGE_READWRITE, 0, size, NULL);
        Assert_(batch->mappingHandle != INVALID_HANDLE_VALUE);

        void* memory = MapViewOfFile(batch->mappingHandle, FILE_MAP_WRITE, 0, 0, 0);
        Assert_(memory != nullptr);

        batch->rays = (DeferredRay*)memory;

        deferredBatches.Add(batch);

        return batch;
    }

    //=================================================================================================================================
    void PathTracingBatcher::FlushCompletedBatch(DeferredBatch* batch)
    {
        if(batch->batchTail == 0) {
            // -- Flush was called on a batch that hadn't been touched at all. We can safely reset this batch here.
            batch->batchHead = 0;
            return;
        }

        if(currentDeferred[batch->category] == batch) {
            currentDeferred[batch->category] = AllocateRayBatch(batch->category);
        }

        UnmapViewOfFile((void*)batch->rays);
        CloseHandle(batch->fileHandle);
        CloseHandle(batch->mappingHandle);

        batch->fileHandle = INVALID_HANDLE_VALUE;
        batch->mappingHandle = INVALID_HANDLE_VALUE;

        readyDeferredBatches.Add(batch);
    }

    //=================================================================================================================================
    void PathTracingBatcher::LoadBatch(DeferredBatch* batch)
    {
        FilePathString filepath = CreateBatchFilePath(batch->batchIndex);

        void* fileData;
        uint64 fileSize;
        Error err = File::ReadWholeFile(filepath.Ascii(), &fileData, &fileSize);
        Assert_(Successful_(err));

        DeleteFileA(filepath.Ascii());

        batch->rays = (DeferredRay*)fileData;
    }

    //=================================================================================================================================
    OcclusionBatch* PathTracingBatcher::AllocateOcclusionBatch(RayBatchCategory category)
    {
        OcclusionBatch* batch = New_(OcclusionBatch);

        batch->batchIndex = Atomic::Increment64(&batchIndex);
        batch->batchHead = 0;
        batch->batchTail = 0;
        batch->category = category;

        FilePathString path = CreateBatchFilePath(batch->batchIndex);

        Directory::EnsureDirectoryExists(path.Ascii());

        batch->fileHandle = CreateFileA(path.Ascii(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);
        Assert_(batch->fileHandle != INVALID_HANDLE_VALUE);

        Assert_(rayBatchCapacity * sizeof(OcclusionRay) < 0xFFFFFFFF);
        DWORD size = (DWORD)(rayBatchCapacity * sizeof(OcclusionRay));

        batch->mappingHandle = CreateFileMapping(batch->fileHandle, NULL, PAGE_READWRITE, 0, size, NULL);
        Assert_(batch->mappingHandle != INVALID_HANDLE_VALUE);

        void* memory = MapViewOfFile(batch->mappingHandle, FILE_MAP_WRITE, 0, 0, 0);
        Assert_(memory != nullptr);

        batch->rays = (OcclusionRay*)memory;

        occlusionBatches.Add(batch);

        return batch;
    }

    //=================================================================================================================================
    void PathTracingBatcher::FlushCompletedBatch(OcclusionBatch* batch)
    {
        if(batch->batchTail == 0) {
            // -- Flush was called on a batch that hadn't been touched at all. We can safely reset this batch here.
            batch->batchHead = 0;
            return;
        }

        if(currentOcclusion[batch->category] == batch) {
            currentOcclusion[batch->category] = AllocateOcclusionBatch(batch->category);
        }

        UnmapViewOfFile((void*)batch->rays);
        CloseHandle(batch->fileHandle);
        CloseHandle(batch->mappingHandle);

        batch->fileHandle = INVALID_HANDLE_VALUE;
        batch->mappingHandle = INVALID_HANDLE_VALUE;

        readyOcclusionBatches.Add(batch);
    }

    //=================================================================================================================================
    void PathTracingBatcher::LoadBatch(OcclusionBatch* batch)
    {
        FilePathString filepath = CreateBatchFilePath(batch->batchIndex);

        void* fileData;
        uint64 fileSize;
        Error err = File::ReadWholeFile(filepath.Ascii(), &fileData, &fileSize);
        Assert_(Successful_(err));

        DeleteFileA(filepath.Ascii());

        batch->rays = (OcclusionRay*)fileData;
    }

    //=================================================================================================================================
    HitBatch* PathTracingBatcher::AllocateHitBatch()
    {
        HitBatch* batch = New_(HitBatch);

        batch->batchIndex = Atomic::Increment64(&batchIndex);
        batch->batchHead = 0;
        batch->batchTail = 0;

        FilePathString path = CreateBatchFilePath(batch->batchIndex);

        Directory::EnsureDirectoryExists(path.Ascii());

        batch->fileHandle = CreateFileA(path.Ascii(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);
        Assert_(batch->fileHandle != INVALID_HANDLE_VALUE);

        Assert_(hitBatchCapacity * sizeof(HitParameters) < 0xFFFFFFFF);
        DWORD size = (DWORD)(hitBatchCapacity * sizeof(HitParameters));

        batch->mappingHandle = CreateFileMapping(batch->fileHandle, NULL, PAGE_READWRITE, 0, size, NULL);
        Assert_(batch->mappingHandle != INVALID_HANDLE_VALUE);

        void* memory = MapViewOfFile(batch->mappingHandle, FILE_MAP_WRITE, 0, 0, 0);
        Assert_(memory != nullptr);

        batch->hits = (HitParameters*)memory;

        hitBatches.Add(batch);

        return batch;
    }

    //=================================================================================================================================
    void PathTracingBatcher::FlushCompletedBatch(HitBatch* batch)
    {
        if(batch->batchTail == 0) {
            // -- Flush was called on a batch that hadn't been touched at all. We can safely reset this batch here.
            batch->batchHead = 0;
            return;
        }

        if(currentHits == batch) {
            currentHits = AllocateHitBatch();
        }

        UnmapViewOfFile((void*)batch->hits);
        CloseHandle(batch->fileHandle);
        CloseHandle(batch->mappingHandle);

        batch->fileHandle = INVALID_HANDLE_VALUE;
        batch->mappingHandle = INVALID_HANDLE_VALUE;

        readyHitBatches.Add(batch);
    }

    //=================================================================================================================================
    void PathTracingBatcher::LoadBatch(HitBatch* batch)
    {
        FilePathString filepath = CreateBatchFilePath(batch->batchIndex);

        void* fileData;
        uint64 fileSize;
        Error err = File::ReadWholeFile(filepath.Ascii(), &fileData, &fileSize);
        Assert_(Successful_(err));

        DeleteFileA(filepath.Ascii());

        batch->hits = (HitParameters*)fileData;
    }

    //=================================================================================================================================
    PathTracingBatcher::PathTracingBatcher()
        : lock(nullptr)
        , batchIndex(0)
        , rayBatchCapacity(0)
        , hitBatchCapacity(0)
        , totalEntriesAdded(0)
        , totalEntriesConsumed(0)
    {

    }

    //=================================================================================================================================
    PathTracingBatcher::~PathTracingBatcher()
    {
        Assert_(lock == nullptr);
    }

    //=================================================================================================================================
    void PathTracingBatcher::Initialize(uint rayBatchCapacity_, uint hitBatchCapacity_)
    {
        rayBatchCapacity = rayBatchCapacity_;
        hitBatchCapacity = hitBatchCapacity_;
        lock = CreateSpinLock();

        for(uint scan = 0; scan < RayBatchCategoryCount; ++scan) {
            currentDeferred[scan] = AllocateRayBatch((RayBatchCategory)scan);
            currentOcclusion[scan] = AllocateOcclusionBatch((RayBatchCategory)scan);
        }

        currentHits = AllocateHitBatch();
    }

    //=================================================================================================================================
    void PathTracingBatcher::Shutdown()
    {
        for(uint scan = 0, count = deferredBatches.Count(); scan < count; ++scan) {
            Delete_(deferredBatches[scan]);
        }
        deferredBatches.Shutdown();

        for(uint scan = 0, count = occlusionBatches.Count(); scan < count; ++scan) {
            Delete_(occlusionBatches[scan]);
        }
        occlusionBatches.Shutdown();

        for(uint scan = 0, count = hitBatches.Count(); scan < count; ++scan) {
            Delete_(hitBatches[scan]);
        }
        hitBatches.Shutdown();

        Assert_(lock != nullptr);
        CloseSpinlock(lock);
        lock = nullptr;
    }

    //=================================================================================================================================
    void PathTracingBatcher::AddUnsortedDeferredRay(const DeferredRay& dray)
    {
        Atomic::AddU64(&totalEntriesAdded, 1);

        RayBatchCategory category = DetermineRayCategory(dray);

        while(true) {
            DeferredBatch* batch = currentDeferred[category];

            int64 head = batch->batchHead;
            if(head >= rayBatchCapacity) {
                // -- we need to wait for the last thread writing to this batch to replace it.
                continue;
            }

            if(Atomic::CompareExchange64(&batch->batchHead, head + 1, head)) {
                
                batch->rays[head] = dray;

                int64 after = Atomic::Add64(&batch->batchTail, 1) + 1;
                if(after == rayBatchCapacity) {
                    EnterSpinLock(lock);
                    FlushCompletedBatch(batch);
                    LeaveSpinLock(lock);
                }

                break;
            }
        }
    }

    //=================================================================================================================================
    void PathTracingBatcher::AddUnsortedOcclusionRay(const OcclusionRay& oray)
    {
        Atomic::AddU64(&totalEntriesAdded, 1);

        RayBatchCategory category = DetermineRayCategory(oray);

        while(true) {
            OcclusionBatch* batch = currentOcclusion[category];

            int64 head = batch->batchHead;
            if(head >= rayBatchCapacity) {
                // -- we need to wait for the last thread writing to this batch to replace it.
                continue;
            }

            if(Atomic::CompareExchange64(&batch->batchHead, head + 1, head)) {

                batch->rays[head] = oray;

                int64 after = Atomic::Add64(&batch->batchTail, 1) + 1;
                if(after == rayBatchCapacity) {
                    EnterSpinLock(lock);
                    FlushCompletedBatch(batch);
                    LeaveSpinLock(lock);
                }

                break;
            }
        }
    }

    //=================================================================================================================================
    void PathTracingBatcher::AddUnsortedHit(const HitParameters& hit)
    {
        Atomic::AddU64(&totalEntriesAdded, 1);

        while(true) {
            HitBatch* batch = currentHits;

            int64 head = batch->batchHead;
            if(head >= hitBatchCapacity) {
                // -- we need to wait for the last thread writing to this batch to replace it.
                continue;
            }

            if(Atomic::CompareExchange64(&batch->batchHead, head + 1, head)) {

                batch->hits[head] = hit;

                int64 after = Atomic::Add64(&batch->batchTail, 1) + 1;
                if(after == hitBatchCapacity) {
                    EnterSpinLock(lock);
                    FlushCompletedBatch(batch);
                    LeaveSpinLock(lock);
                }

                break;
            }
        }
    }

    //=================================================================================================================================
    void PathTracingBatcher::Flush()
    {
        EnterSpinLock(lock);
       
        for(uint scan = 0; scan < RayBatchCategoryCount; ++scan) {
            while(true) {
                DeferredBatch* batch = currentDeferred[scan];
                int64 currentHead = batch->batchHead;
                int64 diff = rayBatchCapacity - currentHead;

                if(diff == 0) {

                    // -- Another thread has already filled this batch so we just wait for that thread to finish the copy.
                    // -- That thread will also call FlushCompletedBatch after we release the lock
                    while(batch->batchTail != rayBatchCapacity) {}
                    break;
                }

                // -- Update head so this batch claims to be at capacity and we know that no other thread will claim space
                if(Atomic::CompareExchange64(&batch->batchHead, currentHead + diff, currentHead)) {

                    // -- Wait until any other threads finish copying their rays into this batch
                    while(batch->batchTail != rayBatchCapacity - diff) {}

                    FlushCompletedBatch(batch);
                    break;
                }
            }
        }

        for(uint scan = 0; scan < RayBatchCategoryCount; ++scan) {
            while(true) {
                OcclusionBatch* batch = currentOcclusion[scan];
                int64 currentHead = batch->batchHead;
                int64 diff = rayBatchCapacity - currentHead;

                if(diff == 0) {
                    // -- Another thread has already filled this batch so we just wait for that thread to finish the copy.
                    // -- That thread will also call FlushCompletedBatch after we release the lock
                    while(batch->batchTail != rayBatchCapacity) {}
                    break;
                }

                // -- Update head so this batch claims to be at capacity and we know that no other thread will claim space
                if(Atomic::CompareExchange64(&batch->batchHead, currentHead + diff, currentHead)) {

                    // -- Wait until any other threads finish copying their rays into this batch
                    while(batch->batchTail != rayBatchCapacity - diff) {}

                    FlushCompletedBatch(batch);
                    break;
                }
            }
        }

        while(true) {
            HitBatch* batch = currentHits;
            int64 currentHead = batch->batchHead;
            int64 diff = hitBatchCapacity - currentHead;

            if(diff == 0) {
                // -- Another thread has already filled this batch so we just wait for that thread to finish the copy.
                // -- That thread will also call FlushCompletedBatch after we release the lock
                while(batch->batchTail != hitBatchCapacity) {}
                break;
            }

            // -- Update head so this batch claims to be at capacity and we know that no other thread will claim space
            if(Atomic::CompareExchange64(&batch->batchHead, currentHead + diff, currentHead)) {

                // -- Wait until any other threads finish copying their rays into this batch
                while(batch->batchTail != hitBatchCapacity - diff) {}

                FlushCompletedBatch(batch);
                break;
            }
        }

        LeaveSpinLock(lock);
    }

    //=================================================================================================================================
    bool PathTracingBatcher::GetSortedBatch(DeferredRay*& rays, uint& rayCount)
    {
        // -- Check before locking to see if it's possible to claim a batch.
        if(readyDeferredBatches.Count() == 0) {
            return false;
        }
        
        EnterSpinLock(lock);

        // -- Check again to make sure another thread didn't claim it before we could enter the lock
        if(readyDeferredBatches.Count() == 0) {
            LeaveSpinLock(lock);
            return false;
        }

        DeferredBatch* batch = readyDeferredBatches[readyDeferredBatches.Count() - 1];
        readyDeferredBatches.RemoveFast(readyDeferredBatches.Count() - 1);

        LeaveSpinLock(lock);

        LoadBatch(batch);

        rays = batch->rays;
        rayCount = (uint)batch->batchTail;

        if(batch->category == PositiveX || batch->category == NegativeX) {
            SortRaysInternal((DeferredRaySortX*)rays, rayCount);
        }
        else if(batch->category == PositiveY || batch->category == NegativeY) {
            SortRaysInternal((DeferredRaySortY*)rays, rayCount);
        }
        else {
            SortRaysInternal((DeferredRaySortZ*)rays, rayCount);
        }

        Atomic::AddU64(&totalEntriesConsumed, rayCount);

        return true;
    }

    //=================================================================================================================================
    void PathTracingBatcher::FreeRays(DeferredRay* rays)
    {
        FreeAligned_(rays);
    }

    //=================================================================================================================================
    bool PathTracingBatcher::GetSortedBatch(OcclusionRay*& rays, uint& rayCount)
    {
        // -- Check before locking to see if it's possible to claim a batch.
        if(readyOcclusionBatches.Count() == 0) {
            return false;
        }

        EnterSpinLock(lock);

        // -- Check again to make sure another thread didn't claim it before we could enter the lock
        if(readyOcclusionBatches.Count() == 0) {
            LeaveSpinLock(lock);
            return false;
        }

        OcclusionBatch* batch = readyOcclusionBatches[readyOcclusionBatches.Count() - 1];
        readyOcclusionBatches.RemoveFast(readyOcclusionBatches.Count() - 1);

        LeaveSpinLock(lock);

        LoadBatch(batch);

        rays = batch->rays;
        rayCount = (uint)batch->batchTail;

        if(batch->category == PositiveX || batch->category == NegativeX) {
            SortRaysInternal((OcclusionRaySortX*)rays, rayCount);
        }
        else if(batch->category == PositiveY || batch->category == NegativeY) {
            SortRaysInternal((OcclusionRaySortY*)rays, rayCount);
        }
        else {
            SortRaysInternal((OcclusionRaySortZ*)rays, rayCount);
        }

        Atomic::AddU64(&totalEntriesConsumed, rayCount);

        return true;
    }

    //=================================================================================================================================
    void PathTracingBatcher::FreeRays(OcclusionRay* rays)
    {
        FreeAligned_(rays);
    }

    //=================================================================================================================================
    bool PathTracingBatcher::GetSortedHits(HitParameters*& hits, uint& hitCount)
    {
        // -- Check before locking to see if it's possible to claim a batch.
        if(readyHitBatches.Count() == 0) {
            return false;
        }

        EnterSpinLock(lock);

        // -- Check again to make sure another thread didn't claim it before we could enter the lock
        if(readyHitBatches.Count() == 0) {
            LeaveSpinLock(lock);
            return false;
        }

        HitBatch* batch = readyHitBatches[readyHitBatches.Count() - 1];
        readyHitBatches.RemoveFast(readyHitBatches.Count() - 1);

        LeaveSpinLock(lock);

        LoadBatch(batch);

        hits = batch->hits;
        hitCount = (uint)batch->batchTail;

        QuickSort(hits, hitCount);

        Atomic::AddU64(&totalEntriesConsumed, hitCount);

        return true;
    }

    //=================================================================================================================================
    void PathTracingBatcher::FreeHits(HitParameters* hits)
    {
        FreeAligned_(hits);
    }

    //=================================================================================================================================
    bool PathTracingBatcher::Empty()
    {
        return (totalEntriesConsumed == totalEntriesAdded);
    }
}
