//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

//#if IsWindows_

#include "SystemLib/MemoryAllocation.h"
#include "SystemLib/OSThreading.h"
#include "SystemLib/BasicTypes.h"
#include "SystemLib/Memory.h"
#include "SystemLib/JsAssert.h"
#include "SystemLib/Logging.h"

#include "StringLib/StringUtil.h"
#include "UtilityLib/QuickSort.h"

#include <stdio.h>
#include <map>
#include <memory>

#if IsWindows_
#include <windows.h>
#endif

// -- allocation tracking
#define EnableManualAllocationTracking_ Debug_ && 1
#define AllocationTrackingIncrement_    4096
#define EnableVerboseLogging_           IsWindows_ && 0
// #define BreakOnAllocation_              61

namespace Selas
{

    #if EnableManualAllocationTracking_
    #define AllocationNameCapacity_ 128

    struct Allocation
    {
        void*       address;
        char        name[AllocationNameCapacity_];
        const char* file;
        int         line;
        uint64      index;
        uint64      size;
    };

    static bool operator<(const Allocation& lhs, const Allocation& rhs)
    {
        return lhs.size < rhs.size;
    }

    static bool operator>(const Allocation& lhs, const Allocation& rhs)
    {
        return lhs.size > rhs.size;
    }

    class CAllocationTracking
    {
    private:
        uint64 index;
        uint   size;
        uint   used;
        Allocation* allocations;
        std::map<void*, uint> allocSearch;

        uint8  spinLock[CacheLineSize_];

        uint64 allocatedMemory;

    public:
        CAllocationTracking();
        ~CAllocationTracking();

        void AddAllocation(void* address, uint64 allocationSize, const char* name, const char* file, int line);
        void RemoveAllocation(void* address);

        void Report();
    };

    //=============================================================================================================================
    CAllocationTracking::CAllocationTracking()
    {
        size = AllocationTrackingIncrement_;
        used = 0;
        allocations = new Allocation[size];
        index = 0;

        allocatedMemory = 0;

        CreateSpinLock(spinLock);
    }

    //=============================================================================================================================
    CAllocationTracking::~CAllocationTracking()
    {
        #if IsWindows_
            char logstring[2048];
        
            for(uint scan = 0; scan < used; ++scan) {
                // -- log each leaked allocation
                if(allocations[scan].name != nullptr) {
                    sprintf_s(logstring, 2048, "Index (%llu) - Name (%s) - Memory leak (%llu bytes) on line (%d) of file: %s\n",
                              allocations[scan].index, allocations[scan].name, allocations[scan].size, allocations[scan].line,
                              allocations[scan].file);
                }
                else {
                    sprintf_s(logstring, 2048, "Index (%llu) - Memory leak (%llu bytes) on line (%d) of file: %s\n",
                              allocations[scan].index, allocations[scan].size, allocations[scan].line, allocations[scan].file);
                }

                OutputDebugStringA(logstring);
            }
        #endif

        AssertMsg_(used == 0, "Some memory allocations were not released properly");
        delete[] allocations;
    }

    //=============================================================================================================================
    void CAllocationTracking::AddAllocation(void* address, uint64 allocationSize, const char* name, const char* file, int line)
    {
        EnterSpinLock(spinLock);

        if(used == size) {
            size += AllocationTrackingIncrement_;

            Allocation* newarray = new Allocation[size];
            Memory::Copy(newarray, allocations, used * sizeof(Allocation));

            delete[] allocations;
            allocations = newarray;
        }

        #ifdef BreakOnAllocation_
            if(BreakOnAllocation_ == index) {
                DebugBreak();
            }
        #endif

        allocSearch.insert(std::pair<void*, uint>(address, used));

        allocations[used].address = address;
        StringUtil::CopyLastN(allocations[used].name, AllocationNameCapacity_, name);
        allocations[used].file = file;
        allocations[used].line = line;
        allocations[used].index = index++;
        allocations[used].size = allocationSize;
        ++used;

        allocatedMemory += allocationSize;

        #if EnableVerboseLogging_
            char logstring[2048];
            sprintf_s(logstring, 2048, "Allocation (%llu): %s - Total Allocated %llu\n", allocationSize, name, allocatedMemory);
            OutputDebugString(logstring);
        #endif

        LeaveSpinLock(spinLock);
    }

    //=============================================================================================================================
    void CAllocationTracking::RemoveAllocation(void* address)
    {
        EnterSpinLock(spinLock);

        auto obj = allocSearch.find(address);
        if(obj == allocSearch.end()) {
            AssertMsg_(false, "Unknown memory address released");
        }
        else {
            uint index = obj->second;
            allocSearch.erase(obj);

            allocatedMemory -= allocations[index].size;

            #if EnableVerboseLogging_
                char logstring[2048];
                sprintf_s(logstring, 2048, "Free (%llu): %s - Total Allocated %llu\n", allocations[scan].size,
                          allocations[scan].name, allocatedMemory);
                OutputDebugString(logstring);
            #endif

            if(used > 1 && index != used - 1) {
                auto& modifiedObj = allocSearch.find(allocations[used - 1].address);
                Assert_(modifiedObj != allocSearch.end());

                Memory::Copy(&allocations[index], &allocations[used - 1], sizeof(Allocation));
                modifiedObj->second = index;
            }

            --used;
        }

        LeaveSpinLock(spinLock);
    }

    //=============================================================================================================================
    void CAllocationTracking::Report()
    {
        EnterSpinLock(spinLock);

        Allocation* allocCopy = (Allocation*)malloc(used * sizeof(Allocation));       
        Memory::Copy(allocCopy, allocations, used * sizeof(Allocation));

        QuickSort(allocCopy, used);

        for(uint scan = 0; scan < used; ++scan) {
            WriteDebugInfo_("%llu - %s", allocCopy[scan].size, allocCopy[scan].name);
        }
        free(allocCopy);

        LeaveSpinLock(spinLock);
    }

    CAllocationTracking tracker;
    #endif

    //=============================================================================================================================
    void* SelasAlignedMalloc(uint size, uint alignment, const char* name, const char* file, int line)
    {
        #if IsWindows_
            void* address = _aligned_malloc(size, alignment);
        #elif IsOsx_
            void* address = nullptr;
            if(posix_memalign(&address, alignment, size) != 0) {
                return nullptr;
            }
        #endif

        #if EnableManualAllocationTracking_
            tracker.AddAllocation(address, size, name, file, line);
        #endif

        return address;
    }

    //=============================================================================================================================
    void* SelasMalloc(uint size, const char* name, const char* file, int line)
    {
        void* address = malloc(size);

        #if EnableManualAllocationTracking_
            tracker.AddAllocation(address, size, name, file, line);
        #endif

        return address;
    }

    //=============================================================================================================================
    void* SelasRealloc(void* address, uint size, const char* name, const char* file, int line)
    {
        #if EnableManualAllocationTracking_
            if(address)
                tracker.RemoveAllocation(address);
        #endif

        address = realloc(address, size);

        #if EnableManualAllocationTracking_
            tracker.AddAllocation(address, size, name, file, line);
        #endif

        return address;
    }

    //=============================================================================================================================
    void SelasAlignedFree(void* address)
    {
        #if EnableManualAllocationTracking_
        if(address)
            tracker.RemoveAllocation(address);
        #endif

        #if IsWindows_
            _aligned_free(address);
        #elif IsOsx_
            // -- posix_memalign just pairs with free.
            free(address);
        #endif
    }

    //=============================================================================================================================
    void SelasFree(void* address)
    {
        #if EnableManualAllocationTracking_
        if(address)
            tracker.RemoveAllocation(address);
        #endif

        free(address);
    }

    //=============================================================================================================================
    void MemoryReport()
    {
        #if EnableManualAllocationTracking_
            tracker.Report();
        #endif
    }
}

//#endif
