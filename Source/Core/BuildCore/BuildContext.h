#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <BuildCore/BuildDependencyGraph.h>

namespace Selas
{

    //==============================================================================
    struct BuildProcessorContext
    {
        BuildId         id;
        BuildIdHash     hash;

        Error AddFileDependency(cpointer file);
        Error AddProcessDependency(const BuildId& id);
        Error AddBuildDependency(const BuildId& dependee, const BuildId& dependency);
        Error CreateOutput(cpointer type, cpointer name, const void* data, uint64 dataSize);

    private:
        BuildProcessDependencies* dependencies;
    };
}
