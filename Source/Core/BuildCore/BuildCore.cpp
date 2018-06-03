//==============================================================================
// Joe Schutte
//==============================================================================

#include <BuildCore/BuildCore.h>
#include <BuildCore/BuildProcessor.h>
#include <UtilityLib/MurmurHash.h>
#include <ThreadingLib/JobMgr.h>
#include <StringLib/StringUtil.h>
#include <SystemLib/JsAssert.h>
#include <SystemLib/MemoryAllocation.h>

#include <map>

namespace Selas
{
    //==============================================================================
    struct BuildCoreData
    {
        std::map<Hash32, CBuildProcessor*> buildProcessors;
    };

    CBuildCore::CBuildCore()
        : _jobMgr(nullptr)
        , _coreData(nullptr)
    {

    }

    //==============================================================================
    CBuildCore::~CBuildCore()
    {
        AssertMsg_(_jobMgr == nullptr, "Shutdown not called on CBuildCore");
        AssertMsg_(_coreData == nullptr, "Shutdown not called on CBuildCore");
    }

    //==============================================================================
    void CBuildCore::Initialize(CJobMgr* jobMgr)
    {
        _jobMgr = jobMgr;

        _coreData = New_(BuildCoreData);
    }

    //==============================================================================
    void CBuildCore::Shutdown()
    {
        SafeDelete_(_coreData);

        _jobMgr = nullptr;
    }

    //==============================================================================
    void CBuildCore::RegisterBuildProcessor(CBuildProcessor* processor)
    {
        const char* patternStr = processor->Pattern();
        uint32 patternLen = StringUtil::Length(patternStr);

        Hash32 pattern = MurmurHash3_x86_32(patternStr, patternLen, 0);
        _coreData->buildProcessors.insert(std::pair<Hash32, CBuildProcessor*>(pattern, processor));
    }
}