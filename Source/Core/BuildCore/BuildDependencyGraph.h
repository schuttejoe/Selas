#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <UtilityLib/MurmurHash.h>
#include <StringLib/FixedString.h>
#include <ContainersLib/CArray.h>
#include <IoLib/FileTime.h>
#include <SystemLib/Error.h>
#include <SystemLib/BasicTypes.h>

namespace Selas
{
    struct BuildGraphData;

    //==============================================================================
    struct BuildId
    {
        bool contentPath;
        FixedString32   type;
        FixedString256  name;
    };

    struct BuildIdHash
    {
        Hash32 type;
        Hash32 name;
    };

    struct FileDependency
    {
        FixedString256 path;
        Hash32         pathHash;
        FileTimestamp  timestamp;
    };

    struct ProcessDependency
    {
        BuildId     id;
        BuildIdHash idHash;
    };

    struct BuildDependency
    {
        BuildId dependeeId;
        BuildIdHash dependeeHash;

        BuildId dependencyId;
        BuildIdHash dependencyHash;
    };

    struct ProcessorOutput
    {
        BuildId id;
        BuildIdHash idHash;
    };

    //==============================================================================
    struct BuildProcessDependencies
    {
        CArray<FileDependency>    fileDependencies;
        CArray<ProcessDependency> processDependencies;
        CArray<BuildDependency>   buildDependencies;
        CArray<ProcessorOutput>   outputs;
    };

    //==============================================================================
    class CBuildDependencyGraph
    {
        CBuildDependencyGraph();
        ~CBuildDependencyGraph();

        Error Initialize();
        Error Shutdown();

        BuildProcessDependencies* Find(BuildId id);

    private:
        BuildGraphData* data;
    };
}
