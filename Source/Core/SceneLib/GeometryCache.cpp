//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SceneLib/GeometryCache.h"
#include "SceneLib/SubsceneResource.h"
#include "SystemLib/OSThreading.h"
#include "SystemLib/Atomic.h"
#include "SystemLib/Logging.h"

namespace Selas
{
    //=============================================================================================================================
    int64 GeometryCache::GetAccessDt()
    {
        return (int64)SystemTime::ElapsedMillisecondsF(startTime);
    }

    //=============================================================================================================================
    void GeometryCache::UnloadLruSubscene()
    {
        int64 lruTimestamp = GetAccessDt();
        int64 lruIndex = -1;

        for(uint scan = 0, count = subscenes.Count(); scan < count; ++scan) {
            if(subscenes[scan]->geometryLoaded && subscenes[scan]->refCount == 0) {
                if(subscenes[scan]->lastAccessDt < lruTimestamp) {
                    lruTimestamp = subscenes[scan]->lastAccessDt;
                    lruIndex = scan;
                }
            }
        }

        if(lruIndex != -1) {
            subscenes[lruIndex]->geometryLoaded = 0;

            // -- This case sucks but we will wait if any threads raised the refcount between when we decided to unload this
            // -- subscene and now.
            while(subscenes[lruIndex]->refCount != 0) {}

            // -- Now we can safely unload it
            WriteDebugInfo_("Unloading subscene %s: ", subscenes[lruIndex]->data->name.Ascii());

            UnloadSubsceneGeometry(subscenes[lruIndex]);

            loadedGeometrySize -= subscenes[lruIndex]->geometrySizeEstimate;
        }
    }

    //=============================================================================================================================
    void GeometryCache::Initialize(uint64 cacheSize)
    {
        loadedGeometrySize = 0;
        loadedGeometryCapacity = cacheSize;
        spinlock = CreateSpinLock();
        startTime = SystemTime::Now();
    }

    //=============================================================================================================================
    void GeometryCache::Shutdown()
    {
        CloseSpinlock(spinlock);
        spinlock = nullptr;
    }

    //=============================================================================================================================
    void GeometryCache::RegisterSubscenes(SubsceneResource** subscenes_, uint64 subsceneCount)
    {
        uint offset = subscenes.Count();
        subscenes.Resize(offset + subsceneCount);

        for(uint scan = 0; scan < subsceneCount; ++scan) {
            subscenes[offset + scan] = subscenes_[scan];
        }
    }

    //=============================================================================================================================
    void GeometryCache::PreloadSubscene(cpointer name)
    {
        uint64 currentSize = loadedGeometrySize;

        for(uint scan = 0, count = subscenes.Count(); scan < count; ++scan) {
            if(StringUtil::EqualsIgnoreCase(subscenes[scan]->data->name.Ascii(), name)) {

                EnsureSubsceneGeometryLoaded(subscenes[scan]);
                break;
            }
        }
     
        uint64 deltaSize = loadedGeometrySize - currentSize;
        Assert_(loadedGeometryCapacity > deltaSize);

        loadedGeometryCapacity -= deltaSize;
        loadedGeometrySize -= deltaSize;

        Assert_(loadedGeometrySize == 0);
    }

    //=============================================================================================================================
    void GeometryCache::EnsureSubsceneGeometryLoaded(SubsceneResource* subscene)
    {
        Atomic::Increment64(&subscene->refCount);

        if(subscene->geometryLoaded == 0 && subscene->geometryLoading == 0) {
            uint64 subsceneSizeEstimate = subscene->geometrySizeEstimate;

            EnterSpinLock(spinlock);

            if(subscene->geometryLoaded == 0 && subscene->geometryLoading == 0) {
                Assert_(subsceneSizeEstimate <= loadedGeometryCapacity);
                while(loadedGeometrySize + subsceneSizeEstimate > loadedGeometryCapacity) {
                    UnloadLruSubscene();
                }

                loadedGeometrySize += subsceneSizeEstimate;

                subscene->geometryLoading = 1;
                LeaveSpinLock(spinlock);

                WriteDebugInfo_("Loading subscene: %s", subscene->data->name.Ascii());
                LoadSubsceneGeometry(subscene);

                subscene->geometryLoading = 0;
            }
            else {
                LeaveSpinLock(spinlock);
            }
        }

        while(subscene->geometryLoading == 1) { }
        Assert_(subscene->geometryLoaded == 1);
    }

    //=============================================================================================================================
    void GeometryCache::FinishUsingSubceneGeometry(SubsceneResource* subscene)
    {
        // -- Try to update the last access timestamp. No big deal if we fail though.
        int64 prevTime = subscene->lastAccessDt;
        int64 updateTime = GetAccessDt();
        Atomic::CompareExchange64(&subscene->lastAccessDt, updateTime, prevTime);

        Atomic::Decrement64(&subscene->refCount);
    }
}