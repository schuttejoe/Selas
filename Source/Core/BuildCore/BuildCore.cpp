//==============================================================================
// Joe Schutte
//==============================================================================

#include "BuildCore/BuildCore.h"
#include "BuildCore/BuildProcessor.h"
#include "BuildCore/BuildContext.h"
#include "UtilityLib/MurmurHash.h"
#include "ThreadingLib/JobMgr.h"
#include "StringLib/StringUtil.h"
#include "SystemLib/JsAssert.h"
#include "SystemLib/MemoryAllocation.h"

#include <map>

namespace Selas
{
    typedef std::map<Hash32, CBuildProcessor*> ProcessorMap;
    typedef std::pair<Hash32, CBuildProcessor*> ProcessorKeyValue;
    typedef std::map<Hash32, CBuildProcessor*>::iterator ProcessorIterator;

    //==============================================================================
    struct BuildCoreData
    {
         ProcessorMap buildProcessors;
         CBuildDependencyGraph* __restrict depGraph;
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
    void CBuildCore::Initialize(CJobMgr* jobMgr, CBuildDependencyGraph* depGraph)
    {
        _jobMgr = jobMgr;

        _coreData = New_(BuildCoreData);
        _coreData->depGraph = depGraph;
    }

    //==============================================================================
    void CBuildCore::Shutdown()
    {
        for(ProcessorIterator it = _coreData->buildProcessors.begin(); it != _coreData->buildProcessors.end(); ++it) {
            Delete_(it->second);
        }

        SafeDelete_(_coreData);

        _jobMgr = nullptr;
    }

    //==============================================================================
    void CBuildCore::RegisterBuildProcessor(CBuildProcessor* processor)
    {
        processor->Setup();

        const char* patternStr = processor->Type();
        uint32 patternLen = StringUtil::Length(patternStr);

        Hash32 pattern = MurmurHash3_x86_32(patternStr, patternLen, 0);
        _coreData->buildProcessors.insert(ProcessorKeyValue(pattern, processor));
    }

    //==============================================================================
    void CBuildCore::BuildAsset(ContentId id)
    {
        Assert_(_coreData != nullptr);
        Assert_(_coreData->depGraph != nullptr);

        ProcessDependency processDep;
        processDep.id = id;

        
    }

    //==============================================================================
    Error CBuildCore::Execute()
    {
        return Success_;
    }
}