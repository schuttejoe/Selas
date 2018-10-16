#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "ContainersLib/CArray.h"
#include "SystemLib/SystemTime.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    struct SubsceneResource;

    //=============================================================================================================================
    class GeometryCache
    {
    private:

        void* spinlock;
        volatile uint64 loadedGeometrySize;
        uint64 loadedGeometryCapacity;
        std::chrono::high_resolution_clock::time_point startTime;

        CArray<SubsceneResource*> subscenes;

        int64 GetAccessDt();
        void UnloadLruSubscene();

    public:

        void Initialize(uint64 cacheSize);
        void Shutdown();

        void RegisterSubscenes(SubsceneResource** subscenes, uint64 subsceneCount);
        void PreloadAll();
        void PreloadSubscene(cpointer name);

        void EnsureSubsceneGeometryLoaded(SubsceneResource* subscene);
        void FinishUsingSubceneGeometry(SubsceneResource* subscene);
    };
}