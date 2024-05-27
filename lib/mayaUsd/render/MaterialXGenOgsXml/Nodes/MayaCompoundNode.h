//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#ifndef MAYA_MATERIALX_COMPOUNDNODE_H
#define MAYA_MATERIALX_COMPOUNDNODE_H

#include <MaterialXGenShader/Nodes/CompoundNode.h>

MATERIALX_NAMESPACE_BEGIN

/// Compound node implementation
class MayaCompoundNode : public CompoundNode
{
public:
    static ShaderNodeImplPtr create();

    void initialize(const InterfaceElement& element, GenContext& context) override;

    void addInputs(ShaderNode& node, GenContext& context) const;

protected:
    StringVec _propagatedInputs;
};

MATERIALX_NAMESPACE_END

#endif
