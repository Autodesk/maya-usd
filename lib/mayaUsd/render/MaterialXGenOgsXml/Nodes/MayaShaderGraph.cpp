//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#include "MayaShaderGraph.h"

#include <MaterialXGenShader/GenContext.h>
#include <MaterialXGenShader/ShaderGenerator.h>
#include <MaterialXGenShader/Util.h>

MATERIALX_NAMESPACE_BEGIN

namespace {
std::string makeValidName(const NodeGraph& nodeGraph, GenContext& context)
{
    NodeDefPtr nodeDef = nodeGraph.getNodeDef();
    if (!nodeDef) {
        throw ExceptionShaderGenError(
            "Can't find nodedef '" + nodeGraph.getNodeDefString() + "' referenced by nodegraph '"
            + nodeGraph.getName() + "'");
    }

    string graphName = nodeGraph.getName();
    context.getShaderGenerator().getSyntax().makeValidName(graphName);
    return graphName;
}

} // namespace

//
// ShaderGraph methods
//
MayaShaderGraph::MayaShaderGraph(
    const ShaderGraph* parent,
    const NodeGraph&   nodeGraph,
    GenContext&        context)
    : ShaderGraph(
        parent,
        makeValidName(nodeGraph, context),
        nodeGraph.getDocument(),
        context.getReservedWords())
{
    // Clear classification
    _classification = 0;

    // Create input sockets from the nodedef
    addInputSockets(*nodeGraph.getNodeDef(), context);

    // Create output sockets from the nodegraph
    addOutputSockets(nodeGraph);

    // Traverse all outputs and create all internal nodes
    for (OutputPtr graphOutput : nodeGraph.getActiveOutputs()) {
        addUpstreamDependencies(*graphOutput, context);
    }

    // Finalize the graph
    finalize(context);
}

MayaShaderGraph::~MayaShaderGraph() = default;

ShaderGraphPtr
MayaShaderGraph::create(const ShaderGraph* parent, const NodeGraph& nodeGraph, GenContext& context)
{
    ShaderGraphPtr graph = std::make_shared<MayaShaderGraph>(parent, nodeGraph, context);

    return graph;
}

void MayaShaderGraph::addPropagatedInput(ShaderNode& node, string const& name)
{
    auto* nodeInput = node.getInput(name);
    if (nodeInput) {
        auto* inputSocket = getInputSocket(name);
        if (!inputSocket) {
            inputSocket = addInputSocket(name, nodeInput->getType());
            _propagatedInputs.push_back(name);
        }
        inputSocket->makeConnection(nodeInput);
        inputSocket->setValue(nodeInput->getValue());
    }
}

StringVec const& MayaShaderGraph::getPropagatedInputs() const { return _propagatedInputs; }

MATERIALX_NAMESPACE_END
