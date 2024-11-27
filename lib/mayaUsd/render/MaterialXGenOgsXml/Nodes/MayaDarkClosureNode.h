//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#ifndef MAYA_MATERIALX_DARKCLOSURENODE_H
#define MAYA_MATERIALX_DARKCLOSURENODE_H

#include <MaterialXGenShader/ShaderNodeImpl.h>

MATERIALX_NAMESPACE_BEGIN

/// Closure node that implements a no-op PBR shading node (with weight zero).
class MayaDarkClosureNode : public ShaderNodeImpl
{
public:
    static ShaderNodeImplPtr create();

    void initialize(const InterfaceElement& element, GenContext& context) override;

    void emitFunctionCall(const ShaderNode& node, GenContext& context, ShaderStage& stage)
        const override;

private:
    bool _isBaseNode = false;
};

MATERIALX_NAMESPACE_END

#endif
