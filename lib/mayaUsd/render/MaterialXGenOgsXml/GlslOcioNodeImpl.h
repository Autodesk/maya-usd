//
// Copyright 2023 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef MATERIALX_GLSLOCIONODEIMPL_H
#define MATERIALX_GLSLOCIONODEIMPL_H

/// @file
/// GLSL OCIO node implementation

#include <mayaUsd/render/MaterialXGenOgsXml/CombinedMaterialXVersion.h>

#include <MaterialXGenGlsl/GlslShaderGenerator.h>

MATERIALX_NAMESPACE_BEGIN

/// GLSL OCIO node implementation. Takes a Maya OCIO shader fragment and
/// makes it compatible with the shadergen
class GlslOcioNodeImpl
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

    void emitFunctionDefinition(const ShaderNode& node, GenContext& context, ShaderStage& stage)
        const override;

    void emitFunctionCall(const ShaderNode& node, GenContext& context, ShaderStage& stage)
        const override;

    /// Prepare all data structures to handle an internal Maya OCIO fragment:
    static std::string registerOCIOFragment(const std::string& fragName);

    /// Get a library with all known internal Maya OCIO fragment:
    static DocumentPtr getOCIOLibrary();

    /// Returns the full list of internal Maya OCIO fragment we can implement:
    static const std::vector<std::string>& getOCIOImplementations();
};

MATERIALX_NAMESPACE_END

#endif
