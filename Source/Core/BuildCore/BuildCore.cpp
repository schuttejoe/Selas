//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCore/BuildCore.h"
#include "BuildCore/BuildProcessor.h"
#include "BuildCore/BuildContext.h"
#include "UtilityLib/MurmurHash.h"
#include "ThreadingLib/JobMgr.h"
#include "StringLib/StringUtil.h"
#include "ContainersLib/QueueList.h"
#include "SystemLib/OSThreading.h"
#include "SystemLib/JsAssert.h"
#include "SystemLib/MemoryAllocation.h"
#include "SystemLib/MinMax.h"

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

        QueueList jobDataFreeList;

        QueueList pendingQueue;

        void* mtQueuesSpinlock;
        QueueList completedQueue;
        QueueList failedQueue;

        JobGroup jobGroup;
    };

    //=============================================================================================================================
    class BuildCoreTask : public tbb::task
    {

    };

    //=============================================================================================================================
    struct BuildCoreJobData
    {
        CBuildProcessor* processor;
        BuildProcessDependencies* deps;
        BuildCoreData* coreData;

        BuildProcessorContext context;
    };

    //=============================================================================================================================
    static BuildCoreJobData* AllocateJobData(BuildCoreData* coreData)
    {
        BuildCoreJobData* jobData = QueueList_Pop<BuildCoreJobData*>(&coreData->jobDataFreeList);
        if(jobData) {
            return jobData;
        }

        return New_(BuildCoreJobData);
    }

    //=============================================================================================================================
    CBuildCore::CBuildCore()
        : _jobMgr(nullptr)
        , _coreData(nullptr)
    {

    }

    //=============================================================================================================================
    CBuildCore::~CBuildCore()
    {
        AssertMsg_(_jobMgr == nullptr, "Shutdown not called on CBuildCore");
        AssertMsg_(_coreData == nullptr, "Shutdown not called on CBuildCore");
    }

    //=============================================================================================================================
    void CBuildCore::Initialize(CJobMgr* jobMgr, CBuildDependencyGraph* depGraph)
    {
        _jobMgr = jobMgr;

        _coreData = New_(BuildCoreData);
        _coreData->depGraph = depGraph;

        QueueList_Initialize(&_coreData->jobDataFreeList, /*maxFreeListSize=*/64);
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

        BuildCoreJobData* jobData = QueueList_Pop<BuildCoreJobData*>(&_coreData->jobDataFreeList);
        while(jobData != nullptr) {
            Delete_(jobData);
            jobData = QueueList_Pop<BuildCoreJobData*>(&_coreData->jobDataFreeList);
        }

        QueueList_Shutdown(&_coreData->jobDataFreeList);
        QueueList_Shutdown(&_coreData->pendingQueue);
        QueueList_Shutdown(&_coreData->completedQueue);
        QueueList_Shutdown(&_coreData->failedQueue);

        CloseSpinlock(_coreData->mtQueuesSpinlock);
        SafeDelete_(_coreData);

        _jobMgr = nullptr;
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
        bool hasPendingJobs = _jobMgr->GroupDone(&_coreData->jobGroup) == false;
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
        BuildCoreJobData* jobData = QueueList_Pop<BuildCoreJobData*>(&_coreData->completedQueue);
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
        
        QueueList_Push(&_coreData->jobDataFreeList, jobData);

        return true;
    }

    //=============================================================================================================================
    static void ExecuteBuildJob(void* userData)
    {
        BuildCoreJobData* jobData = (BuildCoreJobData*)userData;

        Error result = jobData->processor->Process(&jobData->context);

        if(Successful_(result)) {
            EnterSpinLock(jobData->coreData->mtQueuesSpinlock);
            QueueList_Push(&jobData->coreData->completedQueue, jobData);
            LeaveSpinLock(jobData->coreData->mtQueuesSpinlock);
        }
        else {
            EnterSpinLock(jobData->coreData->mtQueuesSpinlock);
            QueueList_Push(&jobData->coreData->failedQueue, jobData);
            LeaveSpinLock(jobData->coreData->mtQueuesSpinlock);
        }
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

        BuildCoreJobData* jobData = AllocateJobData(_coreData);
        jobData->processor = processor;
        jobData->deps = next;
        jobData->coreData = _coreData;
        jobData->context.Initialize(next->source, next->id);

        if(RunMultiThreaded_) {
            _jobMgr->CreateJob(ExecuteBuildJob, jobData, &_coreData->jobGroup);
        }
        else {
            ExecuteBuildJob(jobData);
        }

        return true;
    }
}
