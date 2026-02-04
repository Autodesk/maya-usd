//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#include "MayaShaderGraph.h"

#include <MaterialXGenShader/GenContext.h>
#include <MaterialXGenShader/ShaderGenerator.h>
#include <MaterialXGenShader/ShaderNode.h>
#include <MaterialXGenShader/TypeDesc.h>

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

#if MX_COMBINED_VERSION >= 13810 && MX_COMBINED_VERSION < 13903
//
// ShaderGraph methods
//
MayaShaderGraph::MayaShaderGraph(
    const ShaderGraph* parent,
    const string&      name,
    const ElementPtr&  element,
    GenContext&        context)
    : ShaderGraph(parent, name, element->getDocument(), context.getReservedWords())
    , _shouldPropagateInputs(false)
{
    ElementPtr root;

    if (element->isA<Output>()) {
        OutputPtr  output = element->asA<Output>();
        ElementPtr outputParent = output->getParent();

        InterfaceElementPtr interface;
        if (outputParent->isA<NodeGraph>()) {
            // A nodegraph output.
            NodeGraphPtr nodeGraph = outputParent->asA<NodeGraph>();
            NodeDefPtr   nodeDef = nodeGraph->getNodeDef();
            if (nodeDef) {
                interface = nodeDef;
            } else {
                interface = nodeGraph;
            }
        } else if (outputParent->isA<Document>()) {
            // A free floating output.
            outputParent = output->getConnectedNode();
            interface = outputParent ? outputParent->asA<InterfaceElement>() : nullptr;
        }
        if (!interface) {
            throw ExceptionShaderGenError(
                "Given output '" + output->getName()
                + "' has no interface valid for shader generation");
        }

        // Clear classification
        _classification = 0;

        // Create input sockets
        addInputSockets(*interface, context);

        // Create the given output socket
#if MX_COMBINED_VERSION < 13903
        ShaderGraphOutputSocket* outputSocket
            = addOutputSocket(output->getName(), TypeDesc::get(output->getType()));
#else
        ShaderGraphOutputSocket* outputSocket
            = addOutputSocket(output->getName(), context.getTypeDesc(output->getType()));
#endif
        outputSocket->setPath(output->getNamePath());
        const string& outputUnit = output->getUnit();
        if (!outputUnit.empty()) {
            outputSocket->setUnit(outputUnit);
        }
        const string& outputColorSpace = output->getColorSpace();
        if (!outputColorSpace.empty()) {
            outputSocket->setColorSpace(outputColorSpace);
        }

        // Start traversal from this output
        root = output;
    } else if (element->isA<Node>()) {
        NodePtr    node = element->asA<Node>();
        NodeDefPtr nodeDef = node->getNodeDef();
        if (!nodeDef) {
            throw ExceptionShaderGenError(
                "Could not find a nodedef for node '" + node->getName() + "'");
        }

        // Create input sockets
        addInputSockets(*nodeDef, context);

        // Create output sockets
#if MX_COMBINED_VERSION < 13903
        addOutputSockets(*nodeDef);
#else
        addOutputSockets(*nodeDef, context);
#endif

        // Create this shader node in the graph.
        ShaderNodePtr newNode = ShaderNode::create(this, node->getName(), *nodeDef, context);
        addNode(newNode);

        // Share metadata.
        setMetadata(newNode->getMetadata());

        // Connect it to the graph outputs
        for (size_t i = 0; i < newNode->numOutputs(); ++i) {
            ShaderGraphOutputSocket* outputSocket = getOutputSocket(i);
            outputSocket->makeConnection(newNode->getOutput(i));
            outputSocket->setPath(node->getNamePath());
        }

        // Handle node input ports
        for (const InputPtr& nodedefInput : nodeDef->getActiveInputs()) {
            ShaderGraphInputSocket* inputSocket = getInputSocket(nodedefInput->getName());
            ShaderInput*            input = newNode->getInput(nodedefInput->getName());
            if (!inputSocket || !input) {
                throw ExceptionShaderGenError(
                    "Node input '" + nodedefInput->getName()
                    + "' doesn't match an existing input on graph '" + getName() + "'");
            }

            // Copy data from node element to shadergen representation
            InputPtr nodeInput = node->getInput(nodedefInput->getName());
            if (nodeInput) {
                ValuePtr value = nodeInput->getResolvedValue();
                if (value) {
                    const string& valueString = value->getValueString();
#if MX_COMBINED_VERSION < 13900
                    std::pair<const TypeDesc*, ValuePtr> enumResult;
                    const TypeDesc* type = TypeDesc::get(nodedefInput->getType());
#else
                    std::pair<TypeDesc, ValuePtr> enumResult;
#if MX_COMBINED_VERSION < 13903
                    TypeDesc                      type = TypeDesc::get(nodedefInput->getType());
#else
                    const TypeDesc type = context.getTypeDesc(nodedefInput->getType());
#endif
#endif
                    const string& enumNames
                        = nodedefInput->getAttribute(ValueElement::ENUM_ATTRIBUTE);
                    if (context.getShaderGenerator().getSyntax().remapEnumeration(
                            valueString, type, enumNames, enumResult)) {
                        inputSocket->setValue(enumResult.second);
                    } else {
                        inputSocket->setValue(value);
                    }
                }

                input->setBindInput();
                const string path = nodeInput->getNamePath();
                if (!path.empty()) {
                    inputSocket->setPath(path);
                    input->setPath(path);
                }
                const string& unit = nodeInput->getUnit();
                if (!unit.empty()) {
                    inputSocket->setUnit(unit);
                    input->setUnit(unit);
                }
                const string& colorSpace = nodeInput->getColorSpace();
                if (!colorSpace.empty()) {
                    inputSocket->setColorSpace(colorSpace);
                    input->setColorSpace(colorSpace);
                }
            }

            // Connect graph socket to the node input
            inputSocket->makeConnection(input);

            // Share metadata.
            inputSocket->setMetadata(input->getMetadata());
        }

        // Apply color and unit transforms to each input.
        applyInputTransforms(node, newNode, context);

        // Set root for upstream dependency traversal
        root = node;
    }

    // Traverse and create all dependencies upstream
    if (root && context.getOptions().addUpstreamDependencies) {
        addUpstreamDependencies(*root, context);
    }

    finalize(context);
}
#endif

MayaShaderGraph::MayaShaderGraph(
    const ShaderGraph* parent,
    const NodeGraph&   nodeGraph,
    GenContext&        context)
    : ShaderGraph(
        parent,
        makeValidName(nodeGraph, context),
        nodeGraph.getDocument(),
        context.getReservedWords())
#if MX_COMBINED_VERSION >= 13810 && MX_COMBINED_VERSION < 13903
    , _shouldPropagateInputs(true)
#endif
{
    // Clear classification
    _classification = 0;

    // Create input sockets from the nodedef
    addInputSockets(*nodeGraph.getNodeDef(), context);

    // Create output sockets from the nodegraph
#if MX_COMBINED_VERSION < 13903
    addOutputSockets(nodeGraph);
#else
    addOutputSockets(nodeGraph, context);
#endif

    // Traverse all outputs and create all internal nodes
    for (OutputPtr graphOutput : nodeGraph.getActiveOutputs()) {
        addUpstreamDependencies(*graphOutput, context);
    }

    // Finalize the graph
    finalize(context);
}

MayaShaderGraph::~MayaShaderGraph() = default;

#if MX_COMBINED_VERSION >= 13810 && MX_COMBINED_VERSION < 13903
ShaderGraphPtr MayaShaderGraph::create(
    const ShaderGraph* parent,
    const string&      name,
    ElementPtr         element,
    GenContext&        context)
{
    ShaderGraphPtr graph = std::make_shared<MayaShaderGraph>(parent, name, element, context);
    return graph;
}
#endif

ShaderGraphPtr
MayaShaderGraph::create(const ShaderGraph* parent, const NodeGraph& nodeGraph, GenContext& context)
{
    ShaderGraphPtr graph = std::make_shared<MayaShaderGraph>(parent, nodeGraph, context);

    return graph;
}

void MayaShaderGraph::addPropagatedInput(ShaderNode& node, string const& name)
{
#if MX_COMBINED_VERSION >= 13810 && MX_COMBINED_VERSION < 13903
    if (!_shouldPropagateInputs) {
        return;
    }
#endif

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

#if MX_COMBINED_VERSION >= 13810 && MX_COMBINED_VERSION < 13903
void MayaShaderGraph::createConnectedNodes(
    const ElementPtr& downstreamElement,
    const ElementPtr& upstreamElement,
    ElementPtr        connectingElement,
    GenContext&       context)
{
    // Create the node if it doesn't exist.
    NodePtr upstreamNode = upstreamElement->asA<Node>();
    if (!upstreamNode) {
        throw ExceptionShaderGenError(
            "Upstream element to connect is not a node '" + upstreamElement->getName() + "'");
    }
    const string& newNodeName = upstreamNode->getName();
    ShaderNode*   newNode = getNode(newNodeName);
    if (!newNode) {
        newNode = createNode(upstreamNode, context);
    }

    // Handle interface inputs with default geometric properties.
    for (InputPtr activeInput : upstreamNode->getActiveInputs()) {
        if (!activeInput->hasInterfaceName() || activeInput->getConnectedNode()) {
            continue;
        }

        InputPtr graphInput = activeInput->getInterfaceInput();
        if (graphInput && graphInput->hasDefaultGeomPropString()) {
            ShaderInput* shaderInput
                = getNode(upstreamNode->getName())->getInput(activeInput->getName());
            addDefaultGeomNode(shaderInput, *graphInput->getDefaultGeomProp(), context);
        }
    }

    //
    // Make connections
    //

    // Find the output to connect to.
    if (!connectingElement && downstreamElement->isA<Output>()) {
        // Edge case for having an output downstream but no connecting
        // element (input) reported upstream. In this case we set the
        // output itself as connecting element, which handles finding
        // the nodedef output in case of a multioutput node upstream.
        connectingElement = downstreamElement->asA<Output>();
    }
    OutputPtr nodeDefOutput
        = connectingElement ? upstreamNode->getNodeDefOutput(connectingElement) : nullptr;
    ShaderOutput* output
        = nodeDefOutput ? newNode->getOutput(nodeDefOutput->getName()) : newNode->getOutput();
    if (!output) {
        throw ExceptionShaderGenError(
            "Could not find an output named '"
            + (nodeDefOutput ? nodeDefOutput->getName() : string("out")) + "' on upstream node '"
            + upstreamNode->getName() + "'");
    }

    // Check if it was a node downstream
    NodePtr downstreamNode = downstreamElement->asA<Node>();
    if (downstreamNode) {
        // We have a node downstream
        ShaderNode* downstream = getNode(downstreamNode->getName());
        if (downstream && connectingElement) {
            ShaderInput* input = downstream->getInput(connectingElement->getName());
            if (!input) {
                throw ExceptionShaderGenError(
                    "Could not find an input named '" + connectingElement->getName()
                    + "' on downstream node '" + downstream->getName() + "'");
            }
            input->makeConnection(output);
        } else {
            throw ExceptionShaderGenError(
                "Could not find downstream node ' " + downstreamNode->getName() + "'");
        }
    } else {
        // Not a node, then it must be an output
        ShaderGraphOutputSocket* outputSocket = getOutputSocket(downstreamElement->getName());
        if (outputSocket) {
            outputSocket->makeConnection(output);
        }
    }
}

void MayaShaderGraph::addUpstreamDependencies(const Element& root, GenContext& context)
{
    std::set<ElementPtr> processedOutputs;

    for (Edge edge : root.traverseGraph()) {
        ElementPtr upstreamElement = edge.getUpstreamElement();
        if (!upstreamElement) {
            continue;
        }

        ElementPtr downstreamElement = edge.getDownstreamElement();

        // Early out if downstream element is an output that
        // we have already processed. This might happen since
        // we perform jumps over output elements below.
        if (processedOutputs.count(downstreamElement)) {
            continue;
        }

        // If upstream is an output jump to the actual node connected to the output.
        if (upstreamElement->isA<Output>()) {
            // Record this output so we don't process it again when it
            // shows up as a downstream element in the next iteration.
            processedOutputs.insert(upstreamElement);

            upstreamElement = upstreamElement->asA<Output>()->getConnectedNode();
            if (!upstreamElement) {
                continue;
            }
        }

        createConnectedNodes(
            downstreamElement, upstreamElement, edge.getConnectingElement(), context);
    }
}
#endif

MATERIALX_NAMESPACE_END
