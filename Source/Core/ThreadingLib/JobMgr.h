#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <ThreadingLib/Thread.h>
#include <SystemLib/OsThreading.h>
#include <SystemLib/BasicTypes.h>
#include <SystemLib/JsAssert.h>

namespace Shooty
{
    //==============================================================================
    // Forward declaration
    //==============================================================================
    typedef void(*JobFunction)(void* userData);

    //==============================================================================
    // CJobMgr
    //==============================================================================

    enum
    {
        WorkerThreadCount = 7,
        MaxJobs = 16 * 1024
    };

    struct JobGroup
    {
        int32 groupCount;
        JobGroup() : groupCount(0) {}
    };

    struct Job
    {
        JobFunction  function;
        void*        userData;
        JobGroup*    group;
    };

    class CJobMgr
    {
    public:
        CJobMgr(void);
        ~CJobMgr(void);

        void Initialize(void);
        void Shutdown(void);

        void CreateJob(JobFunction job, void* user_data, JobGroup* group = nullptr);
        void WaitForGroup(JobGroup* group);
        void WaitAll(void);

    private:
        void CreateWorkerThreads(void);

        static void ThreadWorkerFunction(void* user_data);
        static bool ExecuteJob(CJobMgr* job_mgr);

    private:
        Job               allJobs[MaxJobs];
        void*             jobSemaphore;

        void*             jobSpinlock;
        uint32            jobPut;
        uint32            jobGet;

        volatile int32    outstandingJobCount; // Atomic
        volatile bool     terminateThreads;
        ThreadHandle      workerThreads[WorkerThreadCount];
    };
}