//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#include "MayaSourceCodeNode.h"

#include <MaterialXGenShader/HwShaderGenerator.h>

MATERIALX_NAMESPACE_BEGIN

ShaderNodeImplPtr MayaSourceCodeNode::create() { return std::make_shared<MayaSourceCodeNode>(); }

void MayaSourceCodeNode::emitFunctionDefinition(
    const ShaderNode& node,
    GenContext&       context,
    ShaderStage&      stage) const
{
    if (!_sourceFilename.isEmpty()) {
        DEFINE_SHADER_STAGE(stage, Stage::PIXEL)
        {
            // addInclude will make sure not to duplicate the code:
            stage.addInclude(_sourceFilename.getBaseName(), _sourceFilename, context);
        }
        return;
    }

    // Delegate to MaterialX if it was not a simple library include case.
    SourceCodeNode::emitFunctionDefinition(node, context, stage);
}

MATERIALX_NAMESPACE_END
