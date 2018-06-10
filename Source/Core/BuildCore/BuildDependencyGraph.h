#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

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

    //==============================================================================
    struct ContentId
    {
        FixedString32   type;
        FixedString256  name;

        ContentId();
        ContentId(cpointer type_, cpointer name_);
    };

    //==============================================================================
    struct ContentDependency
    {
        FilePathString path;
        FileTimestamp  timestamp;
    };

    //==============================================================================
    struct AssetDependency
    {
        AssetId        id;
        FileTimestamp  timestamp;
    };

    //==============================================================================
    struct ProcessDependency
    {
        ContentId id;
        AssetId   assetId;
    };

    //==============================================================================
    struct ProcessorOutput
    {
        AssetId id;
        uint32 version;
    };

    //==============================================================================
    struct BuildProcessDependencies
    {
        uint32  version;

        CArray<ContentDependency> contentDependencies;
        CArray<ProcessDependency> processDependencies;
        CArray<ProcessorOutput>   outputs;
    };

    //==============================================================================
    class CBuildDependencyGraph
    {
    public:
        CBuildDependencyGraph();
        ~CBuildDependencyGraph();

        Error Initialize();
        Error Shutdown();

        BuildProcessDependencies* Find(AssetId id);
        BuildProcessDependencies* Find(ContentId id);
        BuildProcessDependencies* Create(ContentId id);

    private:
        BuildGraphData* _data;

        friend class CBuildCore;

        bool UpToDate(BuildProcessDependencies* deps);
    };
}
