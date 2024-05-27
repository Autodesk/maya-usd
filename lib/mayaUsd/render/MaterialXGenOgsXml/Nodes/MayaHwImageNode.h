//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#ifndef MAYA_MATERIALX_HWIMAGECODENODE_H
#define MAYA_MATERIALX_HWIMAGECODENODE_H

#include <MaterialXGenShader/Nodes/SourceCodeNode.h>

MATERIALX_NAMESPACE_BEGIN

/// Extending the SourceCodeNode with requirements for image nodes.
class MayaHwImageNode : public SourceCodeNode
{
public:
    static ShaderNodeImplPtr create();

    void addInputs(ShaderNode& node, GenContext& context) const override;
    void setValues(const Node& node, ShaderNode& shaderNode, GenContext& context) const override;
};

MATERIALX_NAMESPACE_END

#endif
