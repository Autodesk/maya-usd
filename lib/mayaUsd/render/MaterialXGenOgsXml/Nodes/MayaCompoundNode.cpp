//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#include "MayaCompoundNode.h"

#include "MayaShaderGraph.h"

#include <MaterialXGenShader/HwShaderGenerator.h>
#include <MaterialXGenShader/ShaderGenerator.h>
#include <MaterialXGenShader/Util.h>

MATERIALX_NAMESPACE_BEGIN

ShaderNodeImplPtr MayaCompoundNode::create() { return std::make_shared<MayaCompoundNode>(); }

void MayaCompoundNode::initialize(const InterfaceElement& element, GenContext& context)
{
    // Copied directly from base class

    ShaderNodeImpl::initialize(element, context);

    if (!element.isA<NodeGraph>()) {
        throw ExceptionShaderGenError(
            "Element '" + element.getName() + "' is not a node graph implementation");
    }

    const NodeGraph& graph = static_cast<const NodeGraph&>(element);

    _functionName = graph.getName();
    context.getShaderGenerator().getSyntax().makeValidName(_functionName);

    // For compounds we do not want to publish all internal inputs
    // so always use the reduced interface for this graph.
    const ShaderInterfaceType oldShaderInterfaceType = context.getOptions().shaderInterfaceType;
    context.getOptions().shaderInterfaceType = SHADER_INTERFACE_REDUCED;

    // Only relevant difference: Use a MayaShaderGraph instead of a ShaderGraph:
    _rootGraph = MayaShaderGraph::create(nullptr, graph, context);
    context.getOptions().shaderInterfaceType = oldShaderInterfaceType;

    // Set hash using the function name.
    // TODO: Could be improved to include the full function signature.
    _hash = std::hash<string> {}(_functionName);
}

void MayaCompoundNode::addInputs(ShaderNode& node, GenContext& context) const
{
    auto const* mayaShaderGraph = dynamic_cast<const MayaShaderGraph*>(getGraph());
    for (auto const& inputName : mayaShaderGraph->getPropagatedInputs()) {
        auto const* inputSocket = mayaShaderGraph->getInputSocket(inputName);
        if (inputSocket && !node.getInput(inputName)) {
            auto* input = node.addInput(inputName, inputSocket->getType());
            input->setValue(inputSocket->getValue());
        }
    }
}

MATERIALX_NAMESPACE_END
