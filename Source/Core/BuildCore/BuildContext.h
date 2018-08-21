#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCore/BuildDependencyGraph.h"
#include "ContainersLib/CSet.h"
#include "SystemLib/BasicTypes.h"

#include "IoLib/BinaryStreamSerializer.h"

namespace Selas
{
    //=============================================================================================================================
    struct BuildProcessorContext
    {
        ContentId source;
        AssetId   id;

        Error AddFileDependency(cpointer file);
        Error AddProcessDependency(const ContentId& id);
        Error AddProcessDependency(cpointer type, cpointer name);
        //Error AddBuildDependency(const BuildId& dependee, const BuildId& dependency);
        Error CreateOutput(cpointer type, uint64 version, cpointer name, const void* data, uint64 dataSize);
        
        template<typename OutputType_>
        Error CreateOutput(cpointer type, uint64 version, cpointer name, OutputType_& serializable);

    private:
        friend class CBuildCore;

        void Initialize(ContentId source, AssetId id);

        CSet<ContentDependency> contentDependencies;
        CSet<ProcessDependency> processDependencies;
        CSet<ProcessorOutput>   outputs;
    };

    template <typename OutputType_>
    Error BuildProcessorContext::CreateOutput(cpointer type, uint64 version, cpointer name, OutputType_& serializable)
    {
        uint8* data;
        uint dataSize;
        SerializeToBinary(serializable, data, dataSize);

        Error err = CreateOutput(type, version, name, data, dataSize);

        FreeAligned_(data);

        return err;
    }
}
