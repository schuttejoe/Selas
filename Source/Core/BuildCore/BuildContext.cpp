//==============================================================================
// Joe Schutte
//==============================================================================

#include "BuildCore/BuildContext.h"
#include "IoLib/File.h"
#include "IoLib/Environment.h"
#include "StringLib/StringUtil.h"

#include <stdio.h>

namespace Selas
{
    //==============================================================================
    Error BuildProcessorContext::AddFileDependency(cpointer file)
    {
        ContentDependency& dep = contentDependencies.Add();

        // JSTODO - Strip the environment root so the asset table can be transfered between machines
        StringUtil::SanitizePath(file, dep.path.Ascii(), dep.path.Capcaity());

        if(FileTime(dep.path.Ascii(), &dep.timestamp) == false) {
            return Error_("Failed to find file: %s", file);
        }

        return Success_;
    }

    //==============================================================================
    Error BuildProcessorContext::AddProcessDependency(const ContentId& id)
    {
        ProcessDependency& dep = processDependencies.Add();
        dep.id = id;
        dep.assetId = AssetId();

        return Success_;
    }

    //==============================================================================
    Error BuildProcessorContext::AddProcessDependency(const AssetId& id)
    {
        ProcessDependency& dep = processDependencies.Add();
        dep.id.type.Clear();
        dep.id.name.Clear();
        dep.assetId = id;

        return Success_;
    }

    //==============================================================================
    //Error BuildProcessorContext::AddBuildDependency(const BuildId& dependee, const BuildId& dependency)
    //{
    //    BuildDependency& dep = dependencies->buildDependencies.Add();
    //    dep.dependeeId   = dependee;
    //    dep.dependencyId = dependency;

    //    return Success_;
    //}

    //==============================================================================
    Error BuildProcessorContext::CreateOutput(cpointer type, uint32 version, cpointer name, const void* data, uint64 dataSize)
    {
        ProcessorOutput& output = outputs.Add();
        output.id = AssetId(type, name);
        output.version = version;

        FilePathString filepath;
        AssetFileUtils::AssetFilePath(output.id, version, filepath);

        ReturnError_(File::WriteWholeFile(filepath.Ascii(), data, (uint32)dataSize));

        return Success_;
    }
}