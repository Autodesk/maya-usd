//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#ifndef MATERIALX_MAYATRANSFORMPOINTNODEGLSL_H
#define MATERIALX_MAYATRANSFORMPOINTNODEGLSL_H

#include "MayaTransformVectorNodeGlsl.h"

MATERIALX_NAMESPACE_BEGIN

/// TransformPoint node implementation for GLSL
class MayaTransformPointNodeGlsl : public MayaTransformVectorNodeGlsl
{
public:
    static ShaderNodeImplPtr create();

protected:
    virtual string
    getHomogeneousCoordinate(const ShaderInput* in, GenContext& context) const override;
};

MATERIALX_NAMESPACE_END

#endif
