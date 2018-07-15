#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCore/BuildDependencyGraph.h"
#include "ContainersLib/CArray.h"
#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    class CJobMgr;
    class CBuildProcessor;

    struct BuildCoreData;

    class CBuildCore
    {
    public:
        CBuildCore();
        ~CBuildCore();

        void Initialize(CJobMgr* jobMgr, CBuildDependencyGraph* depGraph);
        void Shutdown();

        void RegisterBuildProcessor(CBuildProcessor* processor);
        Error BuildAsset(ContentId id);

        Error Execute();

    private:
        CJobMgr*        _jobMgr;
        BuildCoreData*  _coreData;

        void EnqueueInternal(ContentId source, AssetId id);
        void EnqueueDependencies(BuildProcessDependencies* dependencies);

        bool IsBuildComplete();
        bool HasCompletedProcesses();
        
        bool ProcessCompletedQueue();
        bool ProcessPendingQueue();
    };

    template<typename Type_>
    void CreateAndRegisterBuildProcessor(CBuildCore* buildCore)
    {
        Type_* processor = New_(Type_);
        buildCore->RegisterBuildProcessor(processor);
    }
}
