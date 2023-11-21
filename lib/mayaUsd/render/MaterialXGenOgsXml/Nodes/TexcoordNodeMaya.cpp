//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#include "TexcoordNodeMaya.h"

#include <mayaUsd/render/MaterialXGenOgsXml/CombinedMaterialXVersion.h>
#include <mayaUsd/render/MaterialXGenOgsXml/OgsXmlGenerator.h>

#include <MaterialXGenShader/Shader.h>

MATERIALX_NAMESPACE_BEGIN

namespace {
std::string geomNameFromIndex(const std::string& index)
{
    std::string geomname = OgsXmlGenerator::getPrimaryUVSetName();

    // The code below which is trying to handle non-zero indices has little chance of working. Main
    // reason is that our main client (USD) will only handle UV0 by adding an extra primvar in the
    // set of required primvars. Since this is the set that will be used by the mesh hydra scene
    // delegate, the primvars for non-zero index will end up missing in the cached geometry data and
    // prevent copying all necessary buffers to render the material correctly.

    // Fixing this would require extending USD's GetAdditionalPrimvarProperties() to return a
    // functor instead of a hardcoded Token. This way we could add the equivalent of this code as a
    // functor in the MaterialX parser, and the _ExtractPrimvarsFromNode function found in
    // pxr\usdImaging\usdImaging\materialParamUtils.cpp would then be able to handle indexed values.
#if 0
    int i = fromValueString<int>(index);
    if (i < 0 || i > 9) {
        i = 0;
    }
    if (i > 0) {
        if (geomname.back() == '0') {
            // uv0 -> uvX
            geomname.back() = '0' + i;
        } else {
            // st -> stX
            geomname = geomname + std::to_string(i);
        }
    }
#endif
    return geomname;
}
} // namespace

ShaderNodeImplPtr TexcoordNodeGlslMaya::create()
{
    return std::make_shared<TexcoordNodeGlslMaya>();
}

void TexcoordNodeGlslMaya::createVariables(const ShaderNode& node, GenContext&, Shader& shader)
    const
{
    const ShaderInput* indexInput = node.getInput(INDEX);
    if (!indexInput || !indexInput->getValue()) {
        throw ExceptionShaderGenError(
            "No 'index' parameter found on texcoord node '" + node.getName()
            + "'. Don't know what property to bind");
    }
    // Use the standard USD convention for texcoord primvar names:
    const string        index = indexInput ? indexInput->getValue()->getValueString() : "0";
    const string        geomProp = geomNameFromIndex(index);
    const ShaderOutput* output = node.getOutput();

    ShaderStage& vs = shader.getStage(Stage::VERTEX);
    ShaderStage& ps = shader.getStage(Stage::PIXEL);

    addStageInput(HW::VERTEX_INPUTS, output->getType(), HW::T_IN_GEOMPROP + "_" + geomProp, vs);
    addStageConnector(
        HW::VERTEX_DATA, output->getType(), HW::T_IN_GEOMPROP + "_" + geomProp, vs, ps);
}

void TexcoordNodeGlslMaya::emitFunctionCall(
    const ShaderNode& node,
    GenContext&       context,
    ShaderStage&      stage) const
{
    const GlslShaderGenerator& shadergen
        = static_cast<const GlslShaderGenerator&>(context.getShaderGenerator());

    const ShaderInput* indexInput = node.getInput(INDEX);
    if (!indexInput) {
        throw ExceptionShaderGenError(
            "No 'index' parameter found on texcoord node '" + node.getName()
            + "'. Don't know what property to bind");
    }
    // Use the standard USD convention for texcoord primvar names:
    const string index = indexInput ? indexInput->getValue()->getValueString() : "0";
    const string geomname = geomNameFromIndex(index);
    const string variable = HW::T_IN_GEOMPROP + "_" + geomname;

#if MX_COMBINED_VERSION >= 13807
    DEFINE_SHADER_STAGE(stage, Stage::VERTEX)
    {
#else
    BEGIN_SHADER_STAGE(stage, Stage::VERTEX)
#endif
        VariableBlock& vertexData = stage.getOutputBlock(HW::VERTEX_DATA);
        const string   prefix = shadergen.getVertexDataPrefix(vertexData);
        ShaderPort*    geomprop = vertexData[variable];
        if (!geomprop->isEmitted()) {
            shadergen.emitLine(
                prefix + geomprop->getVariable() + " = " + HW::T_IN_GEOMPROP + "_" + geomname,
                stage);
            geomprop->setEmitted();
        }
#if MX_COMBINED_VERSION >= 13807
    }
#else
    END_SHADER_STAGE(shader, Stage::VERTEX)
#endif

#if MX_COMBINED_VERSION >= 13807
    DEFINE_SHADER_STAGE(stage, Stage::PIXEL)
    {
#else
    BEGIN_SHADER_STAGE(stage, Stage::PIXEL)
#endif
        VariableBlock& vertexData = stage.getInputBlock(HW::VERTEX_DATA);
        const string   prefix = shadergen.getVertexDataPrefix(vertexData);
        ShaderPort*    geomprop = vertexData[variable];
        shadergen.emitLineBegin(stage);
        shadergen.emitOutput(node.getOutput(), true, false, context, stage);
        shadergen.emitString(" = " + prefix + geomprop->getVariable(), stage);
        shadergen.emitLineEnd(stage);
#if MX_COMBINED_VERSION >= 13807
    }
#else
    END_SHADER_STAGE(shader, Stage::PIXEL)
#endif
}

MATERIALX_NAMESPACE_END
