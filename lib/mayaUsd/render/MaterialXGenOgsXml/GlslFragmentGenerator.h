//
// TM & (c) 2017 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#ifndef MATERIALX_GLSLFRAGMENTGENERATOR_H
#define MATERIALX_GLSLFRAGMENTGENERATOR_H

/// @file
/// GLSL fragment generator

#include <MaterialXGenGlsl/GlslShaderGenerator.h>
#include <MaterialXGenGlsl/GlslSyntax.h>

MATERIALX_NAMESPACE_BEGIN

namespace Stage {
/// A special stage for private uniform definitions that are not included
/// in the GLSL fragment but need to be known to the GLSL-to-HLSL
/// cross-compiler.
extern const string UNIFORMS;
} // namespace Stage

/// Syntax class for GLSL fragments.
class GlslFragmentSyntax : public GlslSyntax
{
public:
    string getVariableName(const string& name, const TypeDesc* type, IdentifierMap& identifiers)
        const override;
};

using GlslFragmentGeneratorPtr = shared_ptr<class GlslFragmentGenerator>;

/// GLSL shader generator specialized for usage in OGS fragment wrappers.
class GlslFragmentGenerator : public GlslShaderGenerator
{
public:
    GlslFragmentGenerator();

    static ShaderGeneratorPtr create();

    ShaderPtr createShader(const string& name, ElementPtr, GenContext&) const override;
    ShaderPtr generate(const string& name, ElementPtr, GenContext&) const override;

    void emitVariableDeclaration(
        const ShaderPort* variable,
        const string&     qualifier,
        GenContext&,
        ShaderStage&,
        bool assignValue = true) const override;

    void addStageLightingUniforms(GenContext&, ShaderStage&) const override {};

    static const string MATRIX3_TO_MATRIX4_POSTFIX;

protected:
    static void toVec3(const TypeDesc* type, string& variable);
};

MATERIALX_NAMESPACE_END

#endif
