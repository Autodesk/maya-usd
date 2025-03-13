//
// TM & (c) 2017 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#ifndef MATERIALX_GLSLFRAGMENTGENERATOR_H
#define MATERIALX_GLSLFRAGMENTGENERATOR_H

/// @file
/// GLSL fragment generator

#include <mayaUsd/render/MaterialXGenOgsXml/CombinedMaterialXVersion.h>

#include <MaterialXGenGlsl/GlslShaderGenerator.h>
#include <MaterialXGenGlsl/GlslSyntax.h>
#include <MaterialXGenShader/GenUserData.h>

MATERIALX_NAMESPACE_BEGIN

#define MX_REFRACTION_SUBSTITUTION "(mayaGetSpecularEnvironmentNumLOD() > 0)"

namespace Stage {
/// A special stage for private uniform definitions that are not included
/// in the GLSL fragment but need to be known to the GLSL-to-HLSL
/// cross-compiler.
extern const string UNIFORMS;
} // namespace Stage

class HwSpecularEnvironmentSamples;
using HwSpecularEnvironmentSamplesPtr = shared_ptr<class HwSpecularEnvironmentSamples>;
class HwSpecularEnvironmentSamples : public GenUserData
{
public:
    HwSpecularEnvironmentSamples(int numSamples)
        : hwSpecularEnvironmentSamples(numSamples)
    {
    }

    static const std::string& name();

    /// Create and return a new instance.
    static HwSpecularEnvironmentSamplesPtr create(int numSamples)
    {
        return std::make_shared<HwSpecularEnvironmentSamples>(numSamples);
    }

    /// Number of environment samples to take under FIS lighting.
    int hwSpecularEnvironmentSamples = 64;
};

/// Syntax class for GLSL fragments.
class GlslFragmentSyntax : public GlslSyntax
{
public:
#if MX_COMBINED_VERSION < 13900
    string getVariableName(const string& name, const TypeDesc* type, IdentifierMap& identifiers)
#else
    string getVariableName(const string& name, TypeDesc type, IdentifierMap& identifiers)
#endif
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

#ifdef FIX_NODEGRAPH_UDIM_SCALE_OFFSET
    // Locally fixing the UV scale and offset for UDIMs. We will submit to 1.39 later.
    ShaderNodeImplPtr getImplementation(const NodeDef& nodedef, GenContext& context) const override;
#endif

protected:
#if MX_COMBINED_VERSION < 13900
    static void toVec3(const TypeDesc* type, string& variable);
#else
    static void toVec3(const TypeDesc& type, string& variable);
#endif
};

MATERIALX_NAMESPACE_END

#endif
