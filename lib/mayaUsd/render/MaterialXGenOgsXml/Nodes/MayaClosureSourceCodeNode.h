//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#ifndef MAYA_MATERIALX_CLOSURESOURCECODENODE_H
#define MAYA_MATERIALX_CLOSURESOURCECODENODE_H

#include <MaterialXGenShader/Nodes/ClosureSourceCodeNode.h>

MATERIALX_NAMESPACE_BEGIN

/// Source code node that supports the backported OpenPBR Surface node from MaterialX 1.39
class MayaClosureSourceCodeNode : public ClosureSourceCodeNode
{
public:
    static ShaderNodeImplPtr create();

    void emitFunctionDefinition(const ShaderNode& node, GenContext& context, ShaderStage& stage)
        const override;
};

MATERIALX_NAMESPACE_END

#endif
