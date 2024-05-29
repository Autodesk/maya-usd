//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#ifndef MAYA_MATERIALX_SOURCECODENODE_H
#define MAYA_MATERIALX_SOURCECODENODE_H

#include <MaterialXGenShader/Nodes/SourceCodeNode.h>

MATERIALX_NAMESPACE_BEGIN

/// Source code node that backports code duplication fix from
///  https://github.com/AcademySoftwareFoundation/MaterialX/pull/1754 
class MayaSourceCodeNode : public SourceCodeNode
{
public:
    static ShaderNodeImplPtr create();

    void initialize(const InterfaceElement& element, GenContext& context) override;

    void emitFunctionDefinition(const ShaderNode& node, GenContext& context, ShaderStage& stage) const override;

protected:
    FilePath _librarySourceFileName;
};

MATERIALX_NAMESPACE_END

#endif
