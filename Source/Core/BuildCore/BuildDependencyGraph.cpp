//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCore/BuildDependencyGraph.h"
#include "Assets/AssetFileUtils.h"
#include "StringLib/StringUtil.h"
#include "IoLib/Environment.h"
#include "IoLib/File.h"
#include "IoLib/Directory.h"
#include "IoLib/FileTime.h"
#include "IoLib/Serializer.h"
#include "IoLib/SizeSerializer.h"
#include "IoLib/BinarySerializers.h"

#include <map>

namespace Selas
{
    typedef std::map<AssetId, BuildProcessDependencies*>           DependencyMap;
    typedef std::pair<AssetId, BuildProcessDependencies*>          DependencyKeyValue;
    typedef std::map<AssetId, BuildProcessDependencies*>::iterator DependencyIterator;

    #define BuildDependencyGraphType_ "builddependencygraph"
    #define BuildDependencyGraphVersion_ 1535140108ul

    //=============================================================================================================================
    struct BuildGraphData
    {
        DependencyMap dependencyGraph;
    };

    //=============================================================================================================================
    static void BuildGraphFilePath(FilePathString& filepath)
    {
        const char* name = "current";
        AssetFileUtils::AssetFilePath(BuildDependencyGraphType_, BuildDependencyGraphVersion_, name, filepath);
    }

    //=============================================================================================================================
    void Serialize(CSerializer* serializer, BuildProcessDependencies& data)
    {
        Serialize(serializer, data.source);
        Serialize(serializer, data.id);
        Serialize(serializer, data.version);
        Serialize(serializer, data.flags);

        Serialize(serializer, data.contentDependencies);
        Serialize(serializer, data.processDependencies);
        //Serialize(serializer, data.buildDependencies);
        Serialize(serializer, data.outputs);
    }

    //=============================================================================================================================
    static Error LoadDependencyGraph(BuildGraphData* data)
    {
        FilePathString filepath;
        BuildGraphFilePath(filepath);

        if(File::Exists(filepath.Ascii()) == false) {
            return Success_;
        }

        void* fileData;
        uint32 fileSize;
        ReturnError_(File::ReadWholeFile(filepath.Ascii(), &fileData, &fileSize));

        CBinaryReadSerializer* serializer = New_(CBinaryReadSerializer);
        serializer->Initialize((uint8*)fileData, fileSize);

        uint32 count = 0;
        Serialize(serializer, count);
        for(uint32 scan = 0; scan < count; ++scan) {
            BuildProcessDependencies* deps = New_(BuildProcessDependencies);
            Serialize(serializer, *deps);

            // -- clear these flags since they are per-execution data.
            deps->flags &= ~(eEnqueued | eAlreadyBuilt);

            data->dependencyGraph.insert(DependencyKeyValue(deps->id, deps));
        }

        Delete_(serializer);
        FreeAligned_(fileData);

        return Success_;
    }

    //=============================================================================================================================
    static Error SaveDependencyGraph(BuildGraphData* data)
    {
        // -- we can't use SerializeToBinary here since we don't have access to the internals of std::map to write a Serialize
        // -- function for `DependencyMap dependencyGraph`

        FilePathString filepath;
        BuildGraphFilePath(filepath);

        uint32 count = (uint32)data->dependencyGraph.size();
        
        CSizeSerializer* sizeSerializer = New_(CSizeSerializer);      
        Serialize(sizeSerializer, count);
        for(DependencyIterator it = data->dependencyGraph.begin(); it != data->dependencyGraph.end(); ++it) {
            Serialize(sizeSerializer, *it->second);
        }
        uint totalSize = sizeSerializer->TotalSize();
        Delete_(sizeSerializer);

        uint8* memory = AllocArrayAligned_(uint8, totalSize, 16);

        CBinaryWriteSerializer* writeSerializer = New_(CBinaryWriteSerializer);
        writeSerializer->Initialize(memory, totalSize);

        Serialize(writeSerializer, count);
        for(DependencyIterator it = data->dependencyGraph.begin(); it != data->dependencyGraph.end(); ++it) {
            Serialize(writeSerializer, *it->second);
        }
        writeSerializer->SwitchToPtrWrites();
        Serialize(writeSerializer, count);
        for(DependencyIterator it = data->dependencyGraph.begin(); it != data->dependencyGraph.end(); ++it) {
            Serialize(writeSerializer, *it->second);
        }

        Directory::EnsureDirectoryExists(filepath.Ascii());
        ReturnError_(File::WriteWholeFile(filepath.Ascii(), memory, (uint32)totalSize));
        FreeAligned_(memory);
        Delete_(writeSerializer);

        return Success_;
    }

    //=============================================================================================================================
    void ResetBuildProcessDependencies(BuildProcessDependencies* deps)
    {
        deps->version = InvalidIndex32;
        deps->contentDependencies.Shutdown();
        deps->processDependencies.Shutdown();
        deps->outputs.Shutdown();
        deps->flags = 0;
    }

    //=============================================================================================================================
    bool operator==(const ContentDependency& lhs, const ContentDependency& rhs)
    {
        return StringUtil::Equals(lhs.path.Ascii(), rhs.path.Ascii());
    }

    //=============================================================================================================================
    bool operator==(const ProcessDependency& lhs, const ProcessDependency& rhs)
    {
        return (lhs.id.type == rhs.id.type && lhs.id.name == rhs.id.name);
    }

    //=============================================================================================================================
    bool operator==(const ProcessorOutput& lhs, const ProcessorOutput& rhs)
    {
        return (lhs.id.type == rhs.id.type && lhs.id.name == rhs.id.name && lhs.version == rhs.version);
    }

    //=============================================================================================================================
    CBuildDependencyGraph::CBuildDependencyGraph()
        : _data(nullptr)
    {

    }

    //=============================================================================================================================
    CBuildDependencyGraph::~CBuildDependencyGraph()
    {
        Assert_(_data == nullptr);
    }

    //=============================================================================================================================
    Error CBuildDependencyGraph::Initialize()
    {
        _data = New_(BuildGraphData);
        ReturnError_(LoadDependencyGraph(_data));

        return Success_;
    }

    //=============================================================================================================================
    Error CBuildDependencyGraph::Shutdown()
    {
        ReturnError_(SaveDependencyGraph(_data));

        for(DependencyIterator it = _data->dependencyGraph.begin(); it != _data->dependencyGraph.end(); ++it) {
            Delete_(it->second);
        }
        SafeDelete_(_data);

        return Success_;
    }

    //=============================================================================================================================
    BuildProcessDependencies* CBuildDependencyGraph::Find(AssetId id)
    {
        auto obj = _data->dependencyGraph.find(id);
        if(obj != _data->dependencyGraph.end()) {
            return obj->second;
        }

        return nullptr;
    }

    //=============================================================================================================================
    BuildProcessDependencies* CBuildDependencyGraph::Find(ContentId contentId)
    {
        AssetId hashes(contentId.type.Ascii(), contentId.name.Ascii());
        return Find(hashes);
    }

    //=============================================================================================================================
    BuildProcessDependencies* CBuildDependencyGraph::Create(ContentId source)
    {
        AssetId id(source.type.Ascii(), source.name.Ascii());

        Assert_(_data->dependencyGraph.find(id) == _data->dependencyGraph.end());

        BuildProcessDependencies* deps = New_(BuildProcessDependencies);
        deps->id = id;
        deps->source = source;

        _data->dependencyGraph.insert(DependencyKeyValue(id, deps));

        return deps;
    }

    //=============================================================================================================================
    static bool FileUpToDate(const ContentDependency& fileDep)
    {
        FilePathString contentFilePath;
        AssetFileUtils::ContentFilePath(fileDep.path.Ascii(), contentFilePath);

        FileTimestamp timestamp;
        ReturnFailure_(FileTime(contentFilePath.Ascii(), &timestamp));

        return CompareFileTime(fileDep.timestamp, timestamp);
    }

    //=============================================================================================================================
    bool CBuildDependencyGraph::UpToDate(BuildProcessDependencies* __restrict deps, uint64 version)
    {
        if(deps->version != version) {
            return false;
        }

        for(uint scan = 0, count = deps->contentDependencies.Count(); scan < count; ++scan) {
            ReturnFailure_(FileUpToDate(deps->contentDependencies[scan]));
        }

        for(uint scan = 0, count = deps->outputs.Count(); scan < count; ++scan) {
            FilePathString filepath;
            AssetFileUtils::AssetFilePath(deps->outputs[scan].source.type.Ascii(), deps->outputs[scan].version,
                                          deps->outputs[scan].source.name.Ascii(), filepath);

            ReturnFailure_(File::Exists(filepath.Ascii()));
        }

        return true;
    }
}
