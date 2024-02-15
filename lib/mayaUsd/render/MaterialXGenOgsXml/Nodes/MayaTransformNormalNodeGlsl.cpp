//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#include "MayaTransformNormalNodeGlsl.h"

#include <mayaUsd/render/MaterialXGenOgsXml/CombinedMaterialXVersion.h>

MATERIALX_NAMESPACE_BEGIN

ShaderNodeImplPtr MayaTransformNormalNodeGlsl::create()
{
    return std::make_shared<MayaTransformNormalNodeGlsl>();
}

void MayaTransformNormalNodeGlsl::emitFunctionCall(
    const ShaderNode& node,
    GenContext&       context,
    ShaderStage&      stage) const
{
    MayaTransformVectorNodeGlsl::emitFunctionCall(node, context, stage);

#if MX_COMBINED_VERSION >= 13807
    DEFINE_SHADER_STAGE(stage, Stage::PIXEL)
    {
#else
    BEGIN_SHADER_STAGE(stage, Stage::PIXEL)
#endif
        const ShaderGenerator& shadergen = context.getShaderGenerator();
        const ShaderOutput*    output = node.getOutput();
        shadergen.emitLineBegin(stage);
        shadergen.emitOutput(output, false, false, context, stage);
        shadergen.emitString(" = normalize(" + output->getVariable() + ")", stage);
        shadergen.emitLineEnd(stage);
#if MX_COMBINED_VERSION >= 13807
    }
#else
    END_SHADER_STAGE(shader, Stage::PIXEL)
#endif
}

const string&
MayaTransformNormalNodeGlsl::getMatrix(const string& fromSpace, const string& toSpace) const
{
    if ((fromSpace == MODEL || fromSpace == OBJECT) && toSpace == WORLD) {
        return HW::T_WORLD_INVERSE_TRANSPOSE_MATRIX;
    } else if (fromSpace == WORLD && (toSpace == MODEL || toSpace == OBJECT)) {
        return HW::T_WORLD_TRANSPOSE_MATRIX;
    }
    return EMPTY_STRING;
}

MATERIALX_NAMESPACE_END
