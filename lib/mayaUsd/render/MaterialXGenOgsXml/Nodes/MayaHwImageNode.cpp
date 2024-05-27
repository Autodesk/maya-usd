//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#include "MayaHwImageNode.h"

#include "MayaShaderGraph.h"

#include <MaterialXGenShader/HwShaderGenerator.h>
#include <MaterialXGenShader/Util.h>

#include <iostream>

MATERIALX_NAMESPACE_BEGIN
// Additional implementaton arguments for image nodes
const string UV_SCALE = "uv_scale";
const string UV_OFFSET = "uv_offset";

ShaderNodeImplPtr MayaHwImageNode::create() { return std::make_shared<MayaHwImageNode>(); }

void MayaHwImageNode::addInputs(ShaderNode& node, GenContext& context) const
{
    MayaShaderGraph const* graphNode
        = node.getParent() ? dynamic_cast<const MayaShaderGraph*>(node.getParent()) : nullptr;
    // I know, making it non-const is evil, but we are making a workaround here. The final solution
    // in MaterialX 1.39 will have addInputs return the names of the inputs it created.
    MayaShaderGraph* ncGraphNode = graphNode ? const_cast<MayaShaderGraph*>(graphNode) : nullptr;

    // Add additional scale and offset inputs to match implementation arguments
    ShaderInput* scaleInput = node.addInput(UV_SCALE, Type::VECTOR2);
    scaleInput->setValue(Value::createValue<Vector2>(Vector2(1.0f, 1.0f)));
    if (ncGraphNode) {
        ncGraphNode->addPropagatedInput(node, UV_SCALE);
    }
    ShaderInput* offsetInput = node.addInput(UV_OFFSET, Type::VECTOR2);
    offsetInput->setValue(Value::createValue<Vector2>(Vector2(0.0f, 0.0f)));
    if (ncGraphNode) {
        ncGraphNode->addPropagatedInput(node, UV_OFFSET);
    }
}

void MayaHwImageNode::setValues(const Node& node, ShaderNode& shaderNode, GenContext& context) const
{
    // Remap uvs to normalized 0..1 space if the original UDIMs in a UDIM set
    // have been mapped to a single texture atlas which must be accessed in 0..1 space.
    if (context.getOptions().hwNormalizeUdimTexCoords) {
        InputPtr file = node.getInput("file");
        if (file) {
            // set the uv scale and offset properly.
            const string& fileName = file->getValueString();
            if (fileName.find(UDIM_TOKEN) != string::npos) {
                ValuePtr udimSetValue = node.getDocument()->getGeomPropValue(UDIM_SET_PROPERTY);
                if (udimSetValue && udimSetValue->isA<StringVec>()) {
                    const StringVec& udimIdentifiers = udimSetValue->asA<StringVec>();
                    vector<Vector2>  udimCoordinates { getUdimCoordinates(udimIdentifiers) };

                    Vector2 scaleUV { 1.0f, 1.0f };
                    Vector2 offsetUV { 0.0f, 0.0f };
                    getUdimScaleAndOffset(udimCoordinates, scaleUV, offsetUV);

                    ShaderInput* input = shaderNode.getInput(UV_SCALE);
                    if (input) {
                        input->setValue(Value::createValue<Vector2>(scaleUV));
                    }
                    input = shaderNode.getInput(UV_OFFSET);
                    if (input) {
                        input->setValue(Value::createValue<Vector2>(offsetUV));
                    }
                }
            }
        }
    }
}

MATERIALX_NAMESPACE_END
