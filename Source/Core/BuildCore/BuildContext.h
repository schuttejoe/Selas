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
        //Error AddBuildDependency(const BuildId& dependee, const BuildId& dependency);
        Error CreateOutput(cpointer type, uint64 version, cpointer name, const void* data, uint64 dataSize);

    private:
        friend class CBuildCore;

        void Initialize(ContentId source, AssetId id);

        CArray<ContentDependency> contentDependencies;
        CArray<ProcessDependency> processDependencies;
        CArray<ProcessorOutput>   outputs;
    };
}
