//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#include "TexcoordNodeMaya.h"

#include <MaterialXGenShader/Shader.h>

MATERIALX_NAMESPACE_BEGIN

ShaderNodeImplPtr TexcoordNodeGlslMaya::create()
{
    return std::make_shared<TexcoordNodeGlslMaya>();
}

void TexcoordNodeGlslMaya::createVariables(const ShaderNode& node, GenContext&, Shader& shader) const
{
    const ShaderInput* indexInput = node.getInput(INDEX);
    if (!indexInput || !indexInput->getValue())
    {
        throw ExceptionShaderGenError("No 'index' parameter found on texcoord node '" + node.getName() + "'. Don't know what property to bind");
    }
    // Use the standard USD convention for texcoord primvar names:
    const string index = indexInput ? indexInput->getValue()->getValueString() : "0";
    string geomProp = "st";
    if (index != "0") {
        geomProp += index;
    };
    const ShaderOutput* output = node.getOutput();

    ShaderStage& vs = shader.getStage(Stage::VERTEX);
    ShaderStage& ps = shader.getStage(Stage::PIXEL);

    addStageInput(HW::VERTEX_INPUTS, output->getType(), HW::T_IN_GEOMPROP + "_" + geomProp, vs);
    addStageConnector(HW::VERTEX_DATA, output->getType(), HW::T_IN_GEOMPROP + "_" + geomProp, vs, ps);
}

void TexcoordNodeGlslMaya::emitFunctionCall(const ShaderNode& node, GenContext& context, ShaderStage& stage) const
{
    const GlslShaderGenerator& shadergen = static_cast<const GlslShaderGenerator&>(context.getShaderGenerator());

    const ShaderInput* indexInput = node.getInput(INDEX);
    if (!indexInput)
    {
        throw ExceptionShaderGenError("No 'index' parameter found on texcoord node '" + node.getName() + "'. Don't know what property to bind");
    }
    // Use the standard USD convention for texcoord primvar names:
    const string index = indexInput ? indexInput->getValue()->getValueString() : "0";
    string geomname = "st";
    if (index != "0") {
        geomname += index;
    };
    const string variable = HW::T_IN_GEOMPROP + "_" + geomname;

    DEFINE_SHADER_STAGE(stage, Stage::VERTEX)
    {
        VariableBlock& vertexData = stage.getOutputBlock(HW::VERTEX_DATA);
        const string prefix = shadergen.getVertexDataPrefix(vertexData);
        ShaderPort* geomprop = vertexData[variable];
        if (!geomprop->isEmitted())
        {
            shadergen.emitLine(prefix + geomprop->getVariable() + " = " + HW::T_IN_GEOMPROP + "_" + geomname, stage);
            geomprop->setEmitted();
        }
    }

    DEFINE_SHADER_STAGE(stage, Stage::PIXEL)
    {
        VariableBlock& vertexData = stage.getInputBlock(HW::VERTEX_DATA);
        const string prefix = shadergen.getVertexDataPrefix(vertexData);
        ShaderPort* geomprop = vertexData[variable];
        shadergen.emitLineBegin(stage);
        shadergen.emitOutput(node.getOutput(), true, false, context, stage);
        shadergen.emitString(" = " + prefix + geomprop->getVariable(), stage);
        shadergen.emitLineEnd(stage);
    }
}

MATERIALX_NAMESPACE_END
