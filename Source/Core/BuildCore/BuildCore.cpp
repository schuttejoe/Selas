//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCore/BuildCore.h"
#include "BuildCore/BuildProcessor.h"
#include "BuildCore/BuildContext.h"
#include "UtilityLib/MurmurHash.h"
#include "StringLib/StringUtil.h"
#include "ContainersLib/QueueList.h"
#include "SystemLib/OSThreading.h"
#include "SystemLib/JsAssert.h"
#include "SystemLib/MemoryAllocation.h"
#include "SystemLib/MinMax.h"
#include "SystemLib/Atomic.h"

#include <map>

#include <tbb/task.h>

#define RunMultiThreaded_ true

namespace Selas
{
    typedef std::map<Hash32, CBuildProcessor*> ProcessorMap;
    typedef std::pair<Hash32, CBuildProcessor*> ProcessorKeyValue;
    typedef std::map<Hash32, CBuildProcessor*>::iterator ProcessorIterator;

    //=============================================================================================================================
    struct BuildCoreData
    {
        ProcessorMap buildProcessors;
        CBuildDependencyGraph* __restrict depGraph;

        QueueList taskDataFreeList;

        QueueList pendingQueue;

        void* mtQueuesSpinlock;
        QueueList completedQueue;
        QueueList failedQueue;

        int64 activeTaskCount;
    };

    //=============================================================================================================================
    struct BuildCoreTaskData
    {
        CBuildProcessor* processor;
        BuildProcessDependencies* deps;
        BuildCoreData* coreData;
        BuildProcessorContext context;
    };

    //=============================================================================================================================
    class BuildCoreTask : public tbb::task
    {
    public:
        BuildCoreTaskData* data;

        tbb::task* execute()
        {
            Error result = data->processor->Process(&data->context);

            if(Successful_(result)) {
                EnterSpinLock(data->coreData->mtQueuesSpinlock);
                QueueList_Push(&data->coreData->completedQueue, data);
                LeaveSpinLock(data->coreData->mtQueuesSpinlock);
            }
            else {
                EnterSpinLock(data->coreData->mtQueuesSpinlock);
                QueueList_Push(&data->coreData->failedQueue, data);
                LeaveSpinLock(data->coreData->mtQueuesSpinlock);
            }

            Atomic::Decrement64(&data->coreData->activeTaskCount);
            return nullptr;
        }
    };

    //=============================================================================================================================
    static BuildCoreTaskData* AllocateTaskData(BuildCoreData* coreData)
    {
        Atomic::Increment64(&coreData->activeTaskCount);

        BuildCoreTaskData* taskData = QueueList_Pop<BuildCoreTaskData*>(&coreData->taskDataFreeList);
        if(taskData) {
            return taskData;
        }

        return New_(BuildCoreTaskData);
    }

    //=============================================================================================================================
    CBuildCore::CBuildCore()
        : _coreData(nullptr)
    {

    }

    //=============================================================================================================================
    CBuildCore::~CBuildCore()
    {
        AssertMsg_(_coreData == nullptr, "Shutdown not called on CBuildCore");
    }

    //=============================================================================================================================
    void CBuildCore::Initialize(CBuildDependencyGraph* depGraph)
    {
        _coreData = New_(BuildCoreData);
        _coreData->depGraph = depGraph;
        _coreData->activeTaskCount = 0;

        QueueList_Initialize(&_coreData->taskDataFreeList, /*maxFreeListSize=*/64);
        QueueList_Initialize(&_coreData->pendingQueue, /*maxFreeListSize=*/64);
        QueueList_Initialize(&_coreData->failedQueue, /*maxFreeListSize=*/0);

        _coreData->mtQueuesSpinlock = CreateSpinLock();
        QueueList_Initialize(&_coreData->completedQueue, /*maxFreeListSize=*/64);
    }

    //=============================================================================================================================
    void CBuildCore::Shutdown()
    {
        for(ProcessorIterator it = _coreData->buildProcessors.begin(); it != _coreData->buildProcessors.end(); ++it) {
            Delete_(it->second);
        }

        BuildCoreTaskData* taskData = QueueList_Pop<BuildCoreTaskData*>(&_coreData->taskDataFreeList);
        while(taskData != nullptr) {
            Delete_(taskData);
            taskData = QueueList_Pop<BuildCoreTaskData*>(&_coreData->taskDataFreeList);
        }

        QueueList_Shutdown(&_coreData->taskDataFreeList);
        QueueList_Shutdown(&_coreData->pendingQueue);
        QueueList_Shutdown(&_coreData->completedQueue);
        QueueList_Shutdown(&_coreData->failedQueue);

        CloseSpinlock(_coreData->mtQueuesSpinlock);
        SafeDelete_(_coreData);
    }

    //=============================================================================================================================
    void CBuildCore::RegisterBuildProcessor(CBuildProcessor* processor)
    {
        processor->Setup();

        const char* patternStr = processor->Type();
        uint32 patternLen = StringUtil::Length(patternStr);

        Hash32 pattern = MurmurHash3_x86_32(patternStr, patternLen, 0);
        _coreData->buildProcessors.insert(ProcessorKeyValue(pattern, processor));
    }

    //=============================================================================================================================
    Error CBuildCore::BuildAsset(ContentId id)
    {
        Assert_(_coreData != nullptr);
        Assert_(_coreData->depGraph != nullptr);

        AssetId assetId(id.type.Ascii(), id.name.Ascii());
        
        if(_coreData->buildProcessors.find(assetId.type) == _coreData->buildProcessors.end()) {
            return Error_("Failed to find build processor of type %s", id.type.Ascii());
        }

        EnqueueInternal(id, assetId);

        return Success_;
    }

    //=============================================================================================================================
    Error CBuildCore::Execute()
    {
        #define MainThreadIdleBackoffCount_     10
        #define MainThreadMaxIdleTime_          2000
        #define MainThreadStartIdleTime_        200

        // -- Used to Sleep the main thread when it is spinning waiting for other threads to finish.
        uint spinningBackoff = 0;
        uint sleepTimeMs = MainThreadStartIdleTime_;

        while(!IsBuildComplete()) {

            while(ProcessPendingQueue()) {
                spinningBackoff = 0;
                sleepTimeMs = MainThreadStartIdleTime_;
            }

            while(ProcessCompletedQueue()) {
                spinningBackoff = 0;
                sleepTimeMs = MainThreadStartIdleTime_;
            }

            ++spinningBackoff;
            if(spinningBackoff == MainThreadIdleBackoffCount_) {
                Sleep(sleepTimeMs);
                sleepTimeMs *= 2;
                sleepTimeMs = Min<uint>(MainThreadMaxIdleTime_, sleepTimeMs);
            }
        }

        // JSTODO - Handle failed jobs

        return Success_;
    }

    //=============================================================================================================================
    void CBuildCore::EnqueueInternal(ContentId source, AssetId id)
    {
        BuildProcessDependencies* deps = _coreData->depGraph->Find(id);
        if(deps == nullptr) {
            deps = _coreData->depGraph->Create(source);
        }

        if((deps->flags & eEnqueued) == 0 && (deps->flags & eAlreadyBuilt) == 0) {
            deps->flags |= eEnqueued;
            QueueList_Push(&_coreData->pendingQueue, deps);
        }
    }

    //=============================================================================================================================
    void CBuildCore::EnqueueDependencies(BuildProcessDependencies* dependencies)
    {
        for(uint scan = 0, count = dependencies->processDependencies.Count(); scan < count; ++scan) {
            EnqueueInternal(dependencies->processDependencies[scan].source, dependencies->processDependencies[scan].id);
        }

        for(uint scan = 0, count = dependencies->outputs.Count(); scan < count; ++scan) {
            EnqueueInternal(dependencies->outputs[scan].source, dependencies->outputs[scan].id);
        }
    }

    //=============================================================================================================================
    bool CBuildCore::IsBuildComplete()
    {
        bool hasPendingJobs = _coreData->activeTaskCount > 0;
        bool hasPendingWork = QueueList_Empty(&_coreData->pendingQueue) == false;
        bool hasCompletedJob = HasCompletedProcesses();

        return (!hasPendingJobs && !hasPendingWork && !hasCompletedJob);
    }

    //=============================================================================================================================
    bool CBuildCore::HasCompletedProcesses()
    {
        EnterSpinLock(_coreData->mtQueuesSpinlock);
        bool empty = QueueList_Empty(&_coreData->completedQueue);
        LeaveSpinLock(_coreData->mtQueuesSpinlock);

        return empty == false;
    }

    //=============================================================================================================================
    bool CBuildCore::ProcessCompletedQueue()
    {
        EnterSpinLock(_coreData->mtQueuesSpinlock);
        BuildCoreTaskData* jobData = QueueList_Pop<BuildCoreTaskData*>(&_coreData->completedQueue);
        LeaveSpinLock(_coreData->mtQueuesSpinlock);
        if(jobData == nullptr) {
            return false;
        }

        BuildProcessDependencies* dependencies = jobData->deps;
        ResetBuildProcessDependencies(dependencies);

        dependencies->version = jobData->processor->Version();

        dependencies->outputs.Append(jobData->context.outputs);
        dependencies->processDependencies.Append(jobData->context.processDependencies);
        dependencies->contentDependencies.Append(jobData->context.contentDependencies);

        dependencies->flags |= eAlreadyBuilt;
        dependencies->flags &= ~eEnqueued;

        // -- Add dependencies to the work queue
        EnqueueDependencies(dependencies);

        jobData->context.outputs.Shutdown();
        jobData->context.processDependencies.Shutdown();
        jobData->context.contentDependencies.Shutdown();
        
        QueueList_Push(&_coreData->taskDataFreeList, jobData);

        return true;
    }

    //=============================================================================================================================
    bool CBuildCore::ProcessPendingQueue()
    {
        BuildProcessDependencies* next = QueueList_Pop<BuildProcessDependencies*>(&_coreData->pendingQueue);
        if(next == nullptr) {
            return false;
        }

        auto search = _coreData->buildProcessors.find(next->id.type);
        if(search == _coreData->buildProcessors.end()) {
            return true;
        }
        
        CBuildProcessor* processor = search->second;
        if(_coreData->depGraph->UpToDate(next, processor->Version())) {
            EnqueueDependencies(next);
            return true;
        }

        BuildCoreTask& task = *new(tbb::task::allocate_root()) BuildCoreTask();
        task.data = AllocateTaskData(_coreData);

        task.data->processor = processor;
        task.data->deps = next;
        task.data->coreData = _coreData;
        task.data->context.Initialize(next->source, next->id);

        tbb::task::enqueue(task);

        return true;
    }
}
