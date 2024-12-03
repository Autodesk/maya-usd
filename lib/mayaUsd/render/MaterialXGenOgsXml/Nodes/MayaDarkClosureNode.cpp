//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#include "MayaDarkClosureNode.h"

#include <mayaUsd/render/MaterialXGenOgsXml/LobePruner.h>

#include <MaterialXGenShader/HwShaderGenerator.h>

MATERIALX_NAMESPACE_BEGIN

ShaderNodeImplPtr MayaDarkClosureNode::create() { return std::make_shared<MayaDarkClosureNode>(); }

void MayaDarkClosureNode::initialize(const InterfaceElement& element, GenContext& context)
{
    ShaderNodeImpl::initialize(element, context);

    if (!element.isA<Implementation>()) {
        throw ExceptionShaderGenError(
            "Element '" + element.getName() + "' is not an Implementation element");
    }

    const Implementation& impl = static_cast<const Implementation&>(element);
    _isBaseNode = impl.getName()
        == MaterialXMaya::ShaderGenUtil::LobePruner::getDarkBaseImplementationName();
}

void MayaDarkClosureNode::emitFunctionCall(
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

        // We want only the
        //    BSDF metal_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
        // part, and nothing else.
        shadergen.emitLineBegin(stage);
        shadergen.emitOutput(node.getOutput(), true, true, context, stage);
        shadergen.emitLineEnd(stage);

        // For "base" nodes, like
        //   - OrenNayar (+mx39)
        //   - Burley
        //   - Conductor
        //   - Subsurface
        //   - Translucent
        //  we also want:
        //    metal_bsdf_out.throughput = vec3(0.0);
        if (_isBaseNode) {
            shadergen.emitLineBegin(stage);
            shadergen.emitOutput(node.getOutput(), false, false, context, stage);
            shadergen.emitString(".throughput = vec3(0.0)", stage);
            shadergen.emitLineEnd(stage);
        }
#if MX_COMBINED_VERSION >= 13807
    }
#else
    END_SHADER_STAGE(shader, Stage::PIXEL)
#endif
}

MATERIALX_NAMESPACE_END
