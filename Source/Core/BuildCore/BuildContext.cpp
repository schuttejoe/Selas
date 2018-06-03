//==============================================================================
// Joe Schutte
//==============================================================================

#include <BuildCore/BuildContext.h>
#include <IoLib/File.h>
#include <IoLib/Environment.h>
#include <StringLib/StringUtil.h>

#include <stdio.h>

namespace Selas
{
    //==============================================================================
    static Error CalculateFileNameHash(cpointer file, Hash32& hash)
    {
        FixedString256 sanitized;
        if(StringUtil::SanitizePath(file, sanitized.Ascii(), sanitized.Capcaity())) {
            return Error_("Error occurred while sanitizing path: %s", file);
        }

        uint32 length = StringUtil::Length(sanitized.Ascii());

        hash = MurmurHash3_x86_32(sanitized.Ascii(), length, 0);
        return Success_;
    }

    //==============================================================================
    static Error CalculateBuildIdHashes(const BuildId& id, BuildIdHash& hashes)
    {
        ReturnError_(CalculateFileNameHash(id.name.Ascii(), hashes.name));
        hashes.type = MurmurHash3_x86_32(id.type.Ascii(), StringUtil::Length(id.type.Ascii()), 0);

        return Success_;
    }

    //==============================================================================
    static Error FilePathFromId(const BuildId& id, FixedString256& path)
    {
        if(id.contentPath) {
            path = id.name;
            return Success_;
        }

        FixedString128 root = Environment_Root();
        char pathsep = StringUtil::PathSeperator();

        sprintf_s(path.Ascii(), path.Capcaity(), "%s_Assets%c%s%c%s", root.Ascii(), pathsep, id.type.Ascii(), pathsep, id.name.Ascii());

        return Success_;
    }

    //==============================================================================
    Error BuildProcessorContext::AddFileDependency(cpointer file)
    {
        FileDependency& dep = dependencies->fileDependencies.Add();

        StringUtil::SanitizePath(file, dep.path.Ascii(), dep.path.Capcaity());

        if(FileTime(dep.path.Ascii(), &dep.timestamp) == false) {
            return Error_("Failed to find file: %s", file);
        }

        ReturnError_(CalculateFileNameHash(dep.path.Ascii(), dep.pathHash));

        return Success_;
    }

    //==============================================================================
    Error BuildProcessorContext::AddProcessDependency(const BuildId& id)
    {
        ProcessDependency& dep = dependencies->processDependencies.Add();
        dep.id = id;
        
        ReturnError_(CalculateBuildIdHashes(id, dep.idHash));

        return Success_;
    }

    //==============================================================================
    Error BuildProcessorContext::AddBuildDependency(const BuildId& dependee, const BuildId& dependency)
    {
        BuildDependency& dep = dependencies->buildDependencies.Add();
        dep.dependeeId   = dependee;
        dep.dependencyId = dependency;
        ReturnError_(CalculateBuildIdHashes(dependee, dep.dependeeHash));
        ReturnError_(CalculateBuildIdHashes(dependency, dep.dependencyHash));

        return Success_;
    }

    //==============================================================================
    Error BuildProcessorContext::CreateOutput(cpointer type, cpointer name, const void* data, uint64 dataSize)
    {
        ProcessorOutput& output = dependencies->outputs.Add();
        output.id.type.Copy(type);
        output.id.name.Copy(name);
        output.id.contentPath = false;

        ReturnError_(CalculateBuildIdHashes(output.id, output.idHash));

        FixedString256 filepath;
        ReturnError_(FilePathFromId(output.id, filepath));

        ReturnError_(File::WriteWholeFile(filepath.Ascii(), data, (uint32)dataSize));

        return Success_;
    }
}