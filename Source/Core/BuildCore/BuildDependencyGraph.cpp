//==============================================================================
// Joe Schutte
//==============================================================================

#include <BuildCore/BuildDependencyGraph.h>
#include <StringLib/StringUtil.h>
#include <IoLib/Environment.h>
#include <IoLib/File.h>

#include <map>

namespace Selas
{
    //==============================================================================
    struct BuildGraphData
    {
        std::map<BuildIdHash, BuildProcessDependencies*> dependencyGraph;
    };

    //==============================================================================
    static void BuildGraphFilePath(FixedString256& filepath)
    {
        char pathsep = StringUtil::PathSeperator();

        FixedString128 root = Environment_Root();
        
        StringUtil::Sprintf(filepath.Ascii(), (uint32)filepath.Capcaity(), "%s_Assets%cBuildGraph%cDependencyGraph.bin", root.Ascii(), pathsep, pathsep);
    }

    //==============================================================================
    CBuildDependencyGraph::CBuildDependencyGraph()
    {

    }

    //==============================================================================
    CBuildDependencyGraph::~CBuildDependencyGraph()
    {

    }

    //==============================================================================
    Error CBuildDependencyGraph::Initialize()
    {
        FixedString256 filepath;
        BuildGraphFilePath(filepath);

        if(File::Exists(filepath.Ascii()) == false) {
            return Success_;
        }

        // -- load the graph data

        return Success_;
    }

    //==============================================================================
    Error CBuildDependencyGraph::Shutdown()
    {
        FixedString256 filepath;
        BuildGraphFilePath(filepath);

        // -- save the resource graph

        return Success_;
    }

    //==============================================================================
    BuildProcessDependencies* CBuildDependencyGraph::Find(BuildId id)
    {

        return nullptr;
    }
}