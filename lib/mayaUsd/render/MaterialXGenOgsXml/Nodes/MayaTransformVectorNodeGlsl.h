//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#ifndef MATERIALX_MAYATRANSFORMVECTORNODEGLSL_H
#define MATERIALX_MAYATRANSFORMVECTORNODEGLSL_H

#include <mayaUsd/render/MaterialXGenOgsXml/CombinedMaterialXVersion.h>

#include <MaterialXGenGlsl/GlslShaderGenerator.h>

MATERIALX_NAMESPACE_BEGIN

/// TransformVector node implementation for GLSL
class MayaTransformVectorNodeGlsl
#if MX_COMBINED_VERSION >= 13904
    : public HwImplementation
#else
    : public GlslImplementation
#endif
{
public:
    static ShaderNodeImplPtr create();

    void
    createVariables(const ShaderNode& node, GenContext& context, Shader& shader) const override;

    void emitFunctionCall(const ShaderNode& node, GenContext& context, ShaderStage& stage)
        const override;

protected:
    virtual const string& getMatrix(const string& fromSpace, const string& toSpace) const;
    virtual string getHomogeneousCoordinate(const ShaderInput* in, GenContext& context) const;

    // Fixes crash getting a value string that is potentially nullptr. This was fixed in MaterialX
    // pull request 1560, so we only need to use patched nodes until v1.38.9.
    string getPortValueString(const ShaderInput* input) const;
};

MATERIALX_NAMESPACE_END

#endif
