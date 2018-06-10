#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

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
    private:

        CJobMgr*                _jobMgr;
        BuildCoreData*          _coreData;

    public:
        CBuildCore();
        ~CBuildCore();

        void Initialize(CJobMgr* jobMgr, CBuildDependencyGraph* depGraph);
        void Shutdown();

        void RegisterBuildProcessor(CBuildProcessor* processor);

        void BuildAsset(ContentId id);
        Error Execute();

    };
}
