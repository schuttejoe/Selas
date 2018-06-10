#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "BuildCore/BuildDependencyGraph.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    //==============================================================================
    struct BuildProcessorContext
    {
        ContentId source;
        AssetId   id;

        Error AddFileDependency(cpointer file);
        Error AddProcessDependency(const ContentId& id);
        Error AddProcessDependency(const AssetId& id);
        //Error AddBuildDependency(const BuildId& dependee, const BuildId& dependency);
        Error CreateOutput(cpointer type, uint32 version, cpointer name, const void* data, uint64 dataSize);

    private:
        CArray<ContentDependency> contentDependencies;
        CArray<ProcessDependency> processDependencies;
        CArray<ProcessorOutput>   outputs;
    };
}
