//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#include "MayaTransformPointNodeGlsl.h"

#include <MaterialXGenShader/Shader.h>

MATERIALX_NAMESPACE_BEGIN

ShaderNodeImplPtr MayaTransformPointNodeGlsl::create()
{
    return std::make_shared<MayaTransformPointNodeGlsl>();
}

string MayaTransformPointNodeGlsl::getHomogeneousCoordinate(
    const ShaderInput* in,
    GenContext&        context) const
{
    const ShaderGenerator& shadergen = context.getShaderGenerator();
    return "vec4(" + shadergen.getUpstreamResult(in, context) + ", 1.0)";
}

MATERIALX_NAMESPACE_END
