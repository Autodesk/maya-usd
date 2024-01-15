//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#ifndef MATERIALX_MAYATRANSFORMNORMALNODEGLSL_H
#define MATERIALX_MAYATRANSFORMNORMALNODEGLSL_H

#include "MayaTransformVectorNodeGlsl.h"

MATERIALX_NAMESPACE_BEGIN

/// TransformNormal node implementation for GLSL
class MayaTransformNormalNodeGlsl : public MayaTransformVectorNodeGlsl
{
public:
    static ShaderNodeImplPtr create();

protected:
    void emitFunctionCall(const ShaderNode& node, GenContext& context, ShaderStage& stage)
        const override;

    const string& getMatrix(const string& fromSpace, const string& toSpace) const override;
};

MATERIALX_NAMESPACE_END

#endif
