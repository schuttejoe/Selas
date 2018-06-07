#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "ContainersLib/CArray.h"
#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    class CJobMgr;
    class CBuildProcessor;

    class CBuildCore
    {
    private:

        CJobMgr*                _jobMgr;
        CArray<CBuildProcessor*> _processors;

    public:
        CBuildCore();
        ~CBuildCore();

        void Initialize(CJobMgr* jobMgr);
        void Shutdown();

        void RegisterBuildProcessor(CBuildProcessor* processor);
    };
}
