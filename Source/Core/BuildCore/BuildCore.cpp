//==============================================================================
// Joe Schutte
//==============================================================================

#include "BuildCore/BuildCore.h"
#include "ThreadingLib/JobMgr.h"
#include "SystemLib/JsAssert.h"

namespace Selas
{
    CBuildCore::CBuildCore()
        : _jobMgr(nullptr)
    {

    }

    //==============================================================================
    CBuildCore::~CBuildCore()
    {
        AssertMsg_(_jobMgr == nullptr, "Shutdown not called on CBuildCore");
    }

    //==============================================================================
    void CBuildCore::Initialize(CJobMgr* jobMgr)
    {
        _jobMgr = jobMgr;
    }

    //==============================================================================
    void CBuildCore::Shutdown()
    {
        _jobMgr = nullptr;
    }

    //==============================================================================
    void CBuildCore::RegisterBuildProcessor(CBuildProcessor* processor)
    {
        _processors.Add(processor);
    }
}