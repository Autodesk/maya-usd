#ifndef MATERIALX_MAYA_SHADERGENUTIL_H
#define MATERIALX_MAYA_SHADERGENUTIL_H

/// @file
/// Helpers

#include <mayaUsd/base/api.h>
#include <mayaUsd/render/MaterialXGenOgsXml/LobePruner.h>

#include <MaterialXCore/Document.h>

#include <map>

namespace mx = MaterialX;
namespace MaterialXMaya {
namespace ShaderGenUtil {

/// Topo-neutral graph duplicator. Creates a topologically neutral copy of a shading graph:

class MAYAUSD_CORE_PUBLIC TopoNeutralGraph
{
public:
    /*! Creates a barebones TopoNeutralGraph that will process the provided material and generate a
     * topo neutral version of it.
     * @param[in] material the material to process
     */
    explicit TopoNeutralGraph(const mx::ElementPtr& material);

    /*! Creates a TopoNeutralGraph that will process the provided material and generate a topo
     * neutral version of it. It will also substitute lobe pruned categories if a LobePruner is
     * provided.
     * @param[in] material the material to process
     * @param[in] lobePruner an instance of a LobePruner. These are usually singletons that
     * accumulate pruned NodeDefs
     */
    TopoNeutralGraph(const mx::ElementPtr& material, const LobePruner::Ptr& lobePruner);

    /*! Creates a TopoNeutralGraph that will process the provided material and generate a topo
     * neutral version of it. It will also substitute lobe pruned categories if a LobePruner is
     * provided.
     * @param[in] material the material to process
     * @param[in] lobePruner an instance of a LobePruner. These are usually singletons that
     * accumulate pruned NodeDefs
     * @param[in] textured is true if the full material is to be processed. When false, we will
     * generate an untextured topo neutral material instead
     */
    TopoNeutralGraph(
        const mx::ElementPtr&  material,
        const LobePruner::Ptr& lobePruner,
        bool                   textured);
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

    // Get the list of node.attribute paths used by the LobePruner to optimize surface shader nodes
    // found in the material that was processed.
    const mx::StringVec& getOptimizedAttributes() const;

    // As we traverse the shader graph, remember all elements that should be watched for value
    // changes
    enum class ElementType
    {
        eRegular,
        eTopological
    };
    using WatchList = std::map<mx::ElementPtr, ElementType>;

    // Get the watch list gathered while traversing
    const WatchList& getWatchList() const;

    // Will init the nodegraph if it does not currently exists:
    mx::NodeGraphPtr& getNodeGraph();

protected:
    void          computeGraph(const mx::ElementPtr& material, bool textured);
    mx::NodePtr   cloneNode(const mx::Node& node, mx::GraphElement& container);
    mx::OutputPtr findNodeGraphOutput(const mx::Input& input, const std::string& outputName);
    std::string   gatherChannels(const mx::Input& input);
    std::string   gatherOutput(const mx::Input& input);
    std::string   gatherDefaultGeomProp(const mx::Input& input);
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
    // All visited nodes/nodeGraphs elements we should monitor for value changes
    WatchList _watchList;
    // Optional LobePruner that can replace a heavy surface shader with a lightweight version.
    LobePruner::Ptr _lobePruner;
    mx::StringVec   _optimizedAttributes;
};

} // namespace ShaderGenUtil
} // namespace MaterialXMaya

#endif
