#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "Assets/AssetFileUtils.h"
#include "UtilityLib/MurmurHash.h"
#include "StringLib/FixedString.h"
#include "ContainersLib/CArray.h"
#include "IoLib/FileTime.h"
#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    struct BuildGraphData;

    //=============================================================================================================================
    struct ContentDependency
    {
        FilePathString path;
        FileTimestamp  timestamp;
    };

    //=============================================================================================================================
    struct ProcessDependency
    {
        ContentId source;
        AssetId   id;
    };

    //=============================================================================================================================
    struct ProcessorOutput
    {
        ContentId source;
        AssetId id;
        uint64 version;
    };

    enum BuildProcessDependencyFlags
    {
        eEnqueued       = 0x01,
        eAlreadyBuilt   = 0x02
    };

    //=============================================================================================================================
    struct BuildProcessDependencies
    {
        BuildProcessDependencies() : version(InvalidIndex32), flags(0) {}

        ContentId   source;
        AssetId     id;
        uint64      version;
        uint32      flags;

        CArray<ContentDependency> contentDependencies;
        CArray<ProcessDependency> processDependencies;
        CArray<ProcessorOutput>   outputs;
    };

    void ResetBuildProcessDependencies(BuildProcessDependencies* deps);

    bool operator==(const ContentDependency& lhs, const ContentDependency& rhs);
    bool operator==(const ProcessDependency& lhs, const ProcessDependency& rhs);
    bool operator==(const ProcessorOutput& lhs, const ProcessorOutput& rhs);

    //=============================================================================================================================
    class CBuildDependencyGraph
    {
    public:
        CBuildDependencyGraph();
        ~CBuildDependencyGraph();

        Error Initialize();
        Error Shutdown();

        BuildProcessDependencies* Find(AssetId id);
        BuildProcessDependencies* Find(ContentId id);

    private:
        BuildGraphData* _data;

        friend class CBuildCore;

        BuildProcessDependencies* Create(ContentId id);
        bool UpToDate(BuildProcessDependencies* deps, uint64 version);
    };
}
