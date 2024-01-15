//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#include "MayaTransformVectorNodeGlsl.h"

#include <MaterialXGenShader/Shader.h>

MATERIALX_NAMESPACE_BEGIN

ShaderNodeImplPtr MayaTransformVectorNodeGlsl::create()
{
    return std::make_shared<MayaTransformVectorNodeGlsl>();
}

void MayaTransformVectorNodeGlsl::createVariables(
    const ShaderNode& node,
    GenContext&,
    Shader& shader) const
{
    string toSpace = getPortValueString(node.getInput(TO_SPACE));
    string fromSpace = getPortValueString(node.getInput(FROM_SPACE));

    const string& matrix = getMatrix(fromSpace, toSpace);
    if (!matrix.empty()) {
        ShaderStage& ps = shader.getStage(Stage::PIXEL);
        addStageUniform(HW::PRIVATE_UNIFORMS, Type::MATRIX44, matrix, ps);
    }
}

void MayaTransformVectorNodeGlsl::emitFunctionCall(
    const ShaderNode& node,
    GenContext&       context,
    ShaderStage&      stage) const
{
#if MX_COMBINED_VERSION >= 13807
    DEFINE_SHADER_STAGE(stage, Stage::PIXEL)
    {
#else
    BEGIN_SHADER_STAGE(stage, Stage::PIXEL)
#endif
        const ShaderGenerator& shadergen = context.getShaderGenerator();

        const ShaderInput* inInput = node.getInput("in");
        if (inInput->getType() != Type::VECTOR3 && inInput->getType() != Type::VECTOR4) {
            throw ExceptionShaderGenError(
                "Transform node must have 'in' type of vector3 or vector4.");
        }

        string toSpace = getPortValueString(node.getInput(TO_SPACE));
        string fromSpace = getPortValueString(node.getInput(FROM_SPACE));

        shadergen.emitLineBegin(stage);
        shadergen.emitOutput(node.getOutput(), true, false, context, stage);
        shadergen.emitString(" = (", stage);
        const string& matrix = getMatrix(fromSpace, toSpace);
        if (!matrix.empty()) {
            shadergen.emitString(matrix + " * ", stage);
        }
        shadergen.emitString(getHomogeneousCoordinate(inInput, context), stage);
        shadergen.emitString(").xyz", stage);
        shadergen.emitLineEnd(stage);
#if MX_COMBINED_VERSION >= 13807
    }
#else
    END_SHADER_STAGE(shader, Stage::PIXEL)
#endif
}

const string&
MayaTransformVectorNodeGlsl::getMatrix(const string& fromSpace, const string& toSpace) const
{
    if ((fromSpace == MODEL || fromSpace == OBJECT) && toSpace == WORLD) {
        return HW::T_WORLD_MATRIX;
    } else if (fromSpace == WORLD && (toSpace == MODEL || toSpace == OBJECT)) {
        return HW::T_WORLD_INVERSE_MATRIX;
    }
    return EMPTY_STRING;
}

string MayaTransformVectorNodeGlsl::getHomogeneousCoordinate(
    const ShaderInput* in,
    GenContext&        context) const
{
    const ShaderGenerator& shadergen = context.getShaderGenerator();
    return "vec4(" + shadergen.getUpstreamResult(in, context) + ", 0.0)";
}

string MayaTransformVectorNodeGlsl::getPortValueString(const ShaderInput* input) const
{
    if (!input || !input->getValue()) {
        return EMPTY_STRING;
    }
    return input->getValue()->getValueString();
}

MATERIALX_NAMESPACE_END
