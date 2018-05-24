//==============================================================================
// Joe Schutte
//==============================================================================

#include <ThreadingLib/JobMgr.h>
#include <SystemLib/JsAssert.h>
#include <SystemLib/Atomic.h>
#include <SystemLib/Memory.h>
#include <memory.h>

namespace Selas
{
    //==============================================================================
    CJobMgr::CJobMgr(void)
        : jobPut(0)
        , jobGet(0)
        , outstandingJobCount(0)
        , terminateThreads(false)
        , jobSpinlock(nullptr)
        , jobSemaphore(nullptr)
    {

    }

    //==============================================================================
    CJobMgr::~CJobMgr(void)
    {
        Assert_(jobSpinlock == nullptr);
        Assert_(jobSemaphore == nullptr);
    }

    //==============================================================================
    void CJobMgr::Initialize(void)
    {
        jobSpinlock = CreateSpinLock();
        jobSemaphore = CreateSemaphore(0, MaxJobs);

        Memory::Set(allJobs, 0, MaxJobs * sizeof(Job));
        CreateWorkerThreads();
    }

    //==============================================================================
    void CJobMgr::Shutdown(void)
    {
        WaitAll();

        terminateThreads = true;
        PostSemaphore(jobSemaphore, WorkerThreadCount);
        for(int32 scan = 0; scan < WorkerThreadCount; ++scan) {
            ShutdownThread(workerThreads[scan]);
        }

        CloseSemaphore(jobSemaphore);
        CloseSpinlock(jobSpinlock);

        jobSemaphore = nullptr;
        jobSpinlock = nullptr;
    }

    //==============================================================================
    void CJobMgr::CreateJob(JobFunction job, void* userData, JobGroup* group)
    {
        Job newJob;
        newJob.function = job;
        newJob.userData = userData;
        newJob.group = group;

        EnterSpinLock(jobSpinlock);

        uint32 jobIndex = jobPut;
        Assert_(jobIndex < MaxJobs);
        jobPut = (jobPut + 1) % MaxJobs;
        allJobs[jobIndex] = newJob;

        if(group) {
            Atomic::Increment32(&group->groupCount);
        }

        Atomic::Increment32(&outstandingJobCount);
        LeaveSpinLock(jobSpinlock);

        PostSemaphore(jobSemaphore, 1);
    }

    //==============================================================================
    void CJobMgr::WaitForGroup(JobGroup* group)
    {
        Assert_(group);
        volatile int32* groupCount = &group->groupCount;
        while(*groupCount > 0) {}
    }

    //==============================================================================
    void CJobMgr::WaitAll(void)
    {
        volatile int32* outstandingJobs = &outstandingJobCount;
        while(*outstandingJobs > 0) {}
    }

    //==============================================================================
    void CJobMgr::CreateWorkerThreads(void)
    {
        terminateThreads = false;
        for(int32 scan = 0; scan < WorkerThreadCount; ++scan) {
            workerThreads[scan] = CreateThread(ThreadWorkerFunction, this);
        }
    }

    //==============================================================================
    void CJobMgr::ThreadWorkerFunction(void* userData)
    {
        CJobMgr* jobMgr = reinterpret_cast<CJobMgr*>(userData);
        while(ExecuteJob(jobMgr)) {}
    }

    //==============================================================================
    bool CJobMgr::ExecuteJob(CJobMgr* jobMgr)
    {
        WaitForSemaphore(jobMgr->jobSemaphore, 0xFFFFFFFF);

        if(jobMgr->terminateThreads) {
            return false;
        }

        // Get the next job
        EnterSpinLock(jobMgr->jobSpinlock);
        uint32 job_index = jobMgr->jobGet;
        jobMgr->jobGet = (jobMgr->jobGet + 1) % MaxJobs;

        Job job = jobMgr->allJobs[job_index];
        LeaveSpinLock(jobMgr->jobSpinlock);

        // Execute the job
        job.function(job.userData);

        if(job.group) {
            Atomic::Decrement32(&job.group->groupCount);
        }

        // Complete the Job
        Atomic::Decrement32(&jobMgr->outstandingJobCount);

        return true;
    }
}