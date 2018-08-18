//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCore/BuildDependencyGraph.h"
#include "Assets/AssetFileUtils.h"
#include "StringLib/StringUtil.h"
#include "IoLib/Environment.h"
#include "IoLib/File.h"
#include <IoLib/FileTime.h>
#include <IoLib/BinarySerializer.h>

#include <map>

namespace Selas
{
    typedef std::map<AssetId, BuildProcessDependencies*>           DependencyMap;
    typedef std::pair<AssetId, BuildProcessDependencies*>          DependencyKeyValue;
    typedef std::map<AssetId, BuildProcessDependencies*>::iterator DependencyIterator;

    #define BuildDependencyGraphType_ "builddependencygraph"
    #define BuildDependencyGraphVersion_ 1528779796ul

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
    template<typename Type_>
    void WriteArray(BinaryWriter* writer, CArray<Type_>& array)
    {
        uint32 count = array.Count();
        SerializerWrite(writer, &count, sizeof(count));
        SerializerWrite(writer, array.DataPointer(), array.DataSize());
    }

    //=============================================================================================================================
    template<typename Type_>
    void ReadArray(BinaryReader* reader, CArray<Type_>& array)
    {
        uint32 count;
        SerializerRead(reader, &count, sizeof(count));
        array.Resize(count);
        SerializerRead(reader, array.DataPointer(), array.DataSize());
    }

    //=============================================================================================================================
    static void WriteBuildProcessDependency(BinaryWriter* writer, BuildProcessDependencies* deps)
    {
        SerializerWrite(writer, &deps->source, sizeof(deps->source));
        SerializerWrite(writer, &deps->id, sizeof(deps->id));
        SerializerWrite(writer, &deps->version, sizeof(deps->version));

        WriteArray(writer, deps->contentDependencies);
        WriteArray(writer, deps->processDependencies);
        //WriteArray(writer, deps->buildDependencies);
        WriteArray(writer, deps->outputs);
    }

    //=============================================================================================================================
    static void ReadBuildProcessDependency(BinaryReader* reader, BuildProcessDependencies* deps)
    {
        SerializerRead(reader, &deps->source, sizeof(deps->source));
        SerializerRead(reader, &deps->id, sizeof(deps->id));
        SerializerRead(reader, &deps->version, sizeof(deps->version));

        ReadArray(reader, deps->contentDependencies);
        ReadArray(reader, deps->processDependencies);
        //ReadArray(reader, deps->buildDependencies);
        ReadArray(reader, deps->outputs);
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

        BinaryReader reader;
        SerializerStart(&reader, fileData, fileSize);

        uint32 count;
        SerializerRead(&reader, &count, sizeof(count));

        for(uint32 scan = 0; scan < count; ++scan) {
            BuildProcessDependencies* deps = New_(BuildProcessDependencies);

            ReadBuildProcessDependency(&reader, deps);

            data->dependencyGraph.insert(DependencyKeyValue(deps->id, deps));
        }

        SerializerEnd(&reader);
        FreeAligned_(fileData);

        return Success_;
    }

    //=============================================================================================================================
    static Error SaveDependencyGraph(BuildGraphData* data)
    {
        FilePathString filepath;
        BuildGraphFilePath(filepath);

        BinaryWriter writer;
        SerializerStart(&writer, 1024 * 1024, 1024 * 1024);

        uint32 count = (uint32)data->dependencyGraph.size();
        SerializerWrite(&writer, &count, sizeof(count));

        for(DependencyIterator it = data->dependencyGraph.begin(); it != data->dependencyGraph.end(); ++it) {
            WriteBuildProcessDependency(&writer, it->second);
        }

        ReturnError_(SerializerEnd(&writer, filepath.Ascii()));

        return Success_;
    }

    //=============================================================================================================================
    void ResetBuildProcessDependencies(BuildProcessDependencies* deps)
    {
        deps->version = InvalidIndex32;
        deps->contentDependencies.Shutdown();
        deps->processDependencies.Shutdown();
        deps->outputs.Shutdown();
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