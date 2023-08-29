//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#ifndef MATERIALX_TEXCOORDNODEGLSLMAYA_H
#define MATERIALX_TEXCOORDNODEGLSLMAYA_H

#include <MaterialXGenGlsl/GlslShaderGenerator.h>

MATERIALX_NAMESPACE_BEGIN

/// Re-implementation of an index-based texcood node into a geompropvalue using standard USD primvar
/// names
class TexcoordNodeGlslMaya : public GlslImplementation
{
public:
    static ShaderNodeImplPtr create();

    void
    createVariables(const ShaderNode& node, GenContext& context, Shader& shader) const override;

    void emitFunctionCall(const ShaderNode& node, GenContext& context, ShaderStage& stage)
        const override;

    bool isEditable(const ShaderInput& /*input*/) const override { return false; }
};

MATERIALX_NAMESPACE_END

#endif
