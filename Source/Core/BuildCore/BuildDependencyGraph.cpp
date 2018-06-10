//==============================================================================
// Joe Schutte
//==============================================================================

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
    #define BuildDependencyGraphVersion_ 1528587150ul

    //==============================================================================
    struct BuildGraphData
    {
        DependencyMap dependencyGraph;
    };

    //==============================================================================
    static void BuildGraphFilePath(FilePathString& filepath)
    {
        const char* name = "current";
        AssetFileUtils::AssetFilePath(BuildDependencyGraphType_, BuildDependencyGraphVersion_, name, filepath);
    }

    //==============================================================================
    template<typename Type_>
    void WriteArray(BinaryWriter* writer, CArray<Type_>& array)
    {
        uint32 count = array.Length();
        SerializerWrite(writer, &count, sizeof(count));
        SerializerWrite(writer, array.GetData(), array.DataSize());
    }

    //==============================================================================
    template<typename Type_>
    void ReadArray(BinaryReader* reader, CArray<Type_>& array)
    {
        uint32 count;
        SerializerRead(reader, &count, sizeof(count));
        array.Resize(count);
        SerializerRead(reader, array.GetData(), array.DataSize());
    }

    //==============================================================================
    static void WriteBuildProcessDependency(BinaryWriter* writer, AssetId id, BuildProcessDependencies* deps)
    {
        SerializerWrite(writer, &id, sizeof(id));
        SerializerWrite(writer, &deps->version, sizeof(deps->version));

        WriteArray(writer, deps->contentDependencies);
        WriteArray(writer, deps->processDependencies);
        //WriteArray(writer, deps->buildDependencies);
        WriteArray(writer, deps->outputs);
    }

    //==============================================================================
    static void ReadBuildProcessDependency(BinaryReader* reader, AssetId& id, BuildProcessDependencies* deps)
    {
        SerializerRead(reader, &id, sizeof(id));
        SerializerRead(reader, &deps->version, sizeof(deps->version));

        ReadArray(reader, deps->contentDependencies);
        ReadArray(reader, deps->processDependencies);
        //ReadArray(reader, deps->buildDependencies);
        ReadArray(reader, deps->outputs);
    }

    //==============================================================================
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

            AssetId id;
            ReadBuildProcessDependency(&reader, id, deps);

            data->dependencyGraph.insert(DependencyKeyValue(id, deps));
        }

        SerializerEnd(&reader);

        return Success_;
    }

    //==============================================================================
    static Error SaveDependencyGraph(BuildGraphData* data)
    {
        FilePathString filepath;
        BuildGraphFilePath(filepath);

        BinaryWriter writer;
        ReturnError_(SerializerStart(&writer, filepath.Ascii(), 1024 * 1024, 1024 * 1024));

        uint32 count = (uint32)data->dependencyGraph.size();
        SerializerWrite(&writer, &count, sizeof(count));

        for(DependencyIterator it = data->dependencyGraph.begin(); it != data->dependencyGraph.end(); ++it) {
            WriteBuildProcessDependency(&writer, it->first, it->second);
        }

        ReturnError_(SerializerEnd(&writer));

        return Success_;
    }

    //==============================================================================
    ContentId::ContentId()
    {
        type.Clear();
        name.Clear();
    }

    //==============================================================================
    ContentId::ContentId(cpointer type_, cpointer name_)
    {
        type.Copy(type_);
        name.Copy(name_);

        // JSTODO -- Validation that name doesn't contain path separators other than
        // -- '|' and that it doesn't contain the root
    }

    //==============================================================================
    CBuildDependencyGraph::CBuildDependencyGraph()
        : _data(nullptr)
    {

    }

    //==============================================================================
    CBuildDependencyGraph::~CBuildDependencyGraph()
    {
        Assert_(_data == nullptr);
    }

    //==============================================================================
    Error CBuildDependencyGraph::Initialize()
    {
        _data = New_(BuildGraphData);
        ReturnError_(LoadDependencyGraph(_data));

        return Success_;
    }

    //==============================================================================
    Error CBuildDependencyGraph::Shutdown()
    {
        ReturnError_(SaveDependencyGraph(_data));

        for(DependencyIterator it = _data->dependencyGraph.begin(); it != _data->dependencyGraph.end(); ++it) {
            Delete_(it->second);
        }
        SafeDelete_(_data);

        return Success_;
    }

    //==============================================================================
    BuildProcessDependencies* CBuildDependencyGraph::Find(AssetId id)
    {
        auto obj = _data->dependencyGraph.find(id);
        if(obj != _data->dependencyGraph.end()) {
            return obj->second;
        }

        return nullptr;
    }

    //==============================================================================
    BuildProcessDependencies* CBuildDependencyGraph::Find(ContentId contentId)
    {
        AssetId hashes(contentId.type.Ascii(), contentId.name.Ascii());
        return Find(hashes);
    }

    //==============================================================================
    BuildProcessDependencies* CBuildDependencyGraph::Create(ContentId contentId)
    {
        AssetId id(contentId.type.Ascii(), contentId.name.Ascii());
        
        Assert_(_data->dependencyGraph.find(id) == _data->dependencyGraph.end());

        BuildProcessDependencies* deps = New_(BuildProcessDependencies);

        _data->dependencyGraph.insert(DependencyKeyValue(id, deps));

        return deps;
    }

    //==============================================================================
    static bool FileUpToDate(const ContentDependency& fileDep)
    {
        FileTimestamp timestamp;
        ReturnFailure_(FileTime(fileDep.path.Ascii(), &timestamp));

        return CompareFileTime(fileDep.timestamp, timestamp);
    }

    //==============================================================================
    bool CBuildDependencyGraph::UpToDate(BuildProcessDependencies* __restrict deps)
    {
        for(uint scan = 0, count = deps->contentDependencies.Length(); scan < count; ++scan) {
            ReturnFailure_(FileUpToDate(deps->contentDependencies[scan]));
        }

        for(uint scan = 0, count = deps->outputs.Length(); scan < count; ++scan) {
            FilePathString filepath;
            AssetFileUtils::AssetFilePath(deps->outputs[scan].id, deps->outputs[scan].version, filepath);

            ReturnFailure_(File::Exists(filepath.Ascii()));
        }

        return true;
    }
}