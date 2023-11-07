#ifndef MATERIALX_MAYA_SHADERGENUTIL_H
#define MATERIALX_MAYA_SHADERGENUTIL_H

/// @file
/// Helpers

#include <mayaUsd/base/api.h>

#include <MaterialXCore/Document.h>

#include <map>

namespace mx = MaterialX;
namespace MaterialXMaya {
namespace ShaderGenUtil {

/// Topo-neutral graph duplicator. Creates a topologically neutral copy of a shading graph:

class MAYAUSD_CORE_PUBLIC TopoNeutralGraph
{
public:
    explicit TopoNeutralGraph(const mx::ElementPtr& material);
    ~TopoNeutralGraph() = default;

    TopoNeutralGraph() = delete;

    TopoNeutralGraph(const TopoNeutralGraph&) = default;
    TopoNeutralGraph& operator=(const TopoNeutralGraph&) = default;
    TopoNeutralGraph(TopoNeutralGraph&&) = default;
    TopoNeutralGraph& operator=(TopoNeutralGraph&&) = default;

    static bool isTopologicalNodeDef(const mx::NodeDef& nodeDef);

    mx::DocumentPtr           getDocument() const;
    static const std::string& getMaterialName();
    const std::string&        getOriginalPath(const std::string& topoPath) const;

private:
    mx::NodePtr   cloneNode(const mx::Node& node, mx::GraphElement& container);
    mx::OutputPtr findNodeGraphOutput(const mx::Input& input, const std::string& outputName);
    std::string   gatherChannels(const mx::Input& input);
    std::string   gatherOutput(const mx::Input& input);
    void          cloneConnection(
                 const mx::Input&   sourceInput,
                 mx::Node&          destNode,
                 mx::NodePtr&       destConnectedNode,
                 const std::string& channelInfo,
                 const std::string& output);
    void cloneNodeGraphConnection(
        const mx::Input&   sourceInput,
        mx::Node&          destNode,
        mx::NodePtr&       destConnectedNode,
        const std::string& channelInfo,
        const std::string& output);

    // The topo neutral document we are trying to create.
    mx::DocumentPtr _doc;
    // This topo neutral document will store all ancillary nodes in a NodeGraph
    mx::NodeGraphPtr _nodeGraph;
    // Since we anonymize the node names, we need a map from original name
    // to the duplicated node.
    using TNodeMap = std::map<std::string, mx::NodePtr>;
    TNodeMap _nodeMap;
    size_t   _nodeIndex = 0;
    // We also make sure to create the minimal number of outputs on the NodeGraph.
    using TOutputMap = std::map<std::string, mx::OutputPtr>;
    TOutputMap _outputMap;
    size_t     _outputIndex = 0;
    // String mapping from topo path to original path:
    std::unordered_map<std::string, std::string> _pathMap;
};

} // namespace ShaderGenUtil
} // namespace MaterialXMaya

#endif
