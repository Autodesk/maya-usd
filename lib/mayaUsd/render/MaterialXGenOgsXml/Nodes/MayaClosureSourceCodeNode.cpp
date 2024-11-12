//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#include "MayaClosureSourceCodeNode.h"

#include <MaterialXGenShader/HwShaderGenerator.h>

MATERIALX_NAMESPACE_BEGIN

ShaderNodeImplPtr MayaClosureSourceCodeNode::create()
{
    return std::make_shared<MayaClosureSourceCodeNode>();
}

void MayaClosureSourceCodeNode::emitFunctionDefinition(
    const ShaderNode& node,
    GenContext&       context,
    ShaderStage&      stage) const
{
    // Pre-inject backported OpenPBR lighting code:
    if (_name == "IM_dielectric_tf_bsdf_genglsl"
        || _name == "IM_generalized_schlick_tf_82_bsdf_genglsl") {
        static const auto allIncludes
            = std::array<FilePath, 2> { "pbrlib/genglsl/lib/mx39_microfacet_specular.glsl",
                                        "pbrlib/genglsl/ogsxml/mx39_lighting_maya_all.glsl" };
        for (auto const& toInclude : allIncludes) {
            // Update source code to inject our mx39 lighting functions:
            FilePath libraryPrefix = context.getOptions().libraryPrefix;
            FilePath fullFilename = libraryPrefix.isEmpty() ? toInclude : libraryPrefix / toInclude;
            FilePath resolvedFilename = context.resolveSourceFile(fullFilename, FilePath());
            stage.addInclude(fullFilename, resolvedFilename, context);
        }
    }
    return ClosureSourceCodeNode::emitFunctionDefinition(node, context, stage);
}

MATERIALX_NAMESPACE_END
