//
// Copyright 2021 Autodesk
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

#include "mtlxBaseWriter.h"

#include <mayaUsd/fileio/shaderReaderRegistry.h>
#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/shading/symmetricShaderWriter.h>
#include <mayaUsd/fileio/utils/roundTripUtil.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/converter.h>

#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>

#include <maya/MFnAttribute.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>

#include <unordered_map>

using namespace MAYAUSD_NS;

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // The MaterialX shader id of standard surface:
    (ND_standard_surface_surfaceshader)

    // All ND_standard_surface_surfaceshader attributes:
    (base)
    (base_color)
    (diffuse_roughness)
    (metalness)
    (specular)
    (specular_color)
    (specular_roughness)
    (specular_IOR)
    (specular_anisotropy)
    (specular_rotation)
    (transmission)
    (transmission_color)
    (transmission_depth)
    (transmission_scatter)
    (transmission_scatter_anisotropy)
    (transmission_dispersion)
    (transmission_extra_roughness)
    (subsurface)
    (subsurface_color)
    (subsurface_radius)
    (subsurface_scale)
    (subsurface_anisotropy)
    (sheen)
    (sheen_color)
    (sheen_roughness)
    (coat)
    (coat_color)
    (coat_roughness)
    (coat_anisotropy)
    (coat_rotation)
    (coat_IOR)
    (coat_normal)
    (coat_affect_color)
    (coat_affect_roughness)
    (thin_film_thickness)
    (thin_film_IOR)
    (emission)
    (emission_color)
    (opacity)
    (thin_walled)
    (normal)
    (tangent)

    // CamelCase versions, as known to Maya:
    (baseColor)
    (diffuseRoughness)
    (specularColor)
    (specularRoughness)
    (specularIOR)
    (specularAnisotropy)
    (specularRotation)
    (transmissionColor)
    (transmissionDepth)
    (transmissionScatter)
    (transmissionScatterAnisotropy)
    (transmissionDispersion)
    (transmissionExtraRoughness)
    (subsurfaceColor)
    (subsurfaceRadius)
    (subsurfaceScale)
    (subsurfaceAnisotropy)
    (sheenColor)
    (sheenRoughness)
    (coatColor)
    (coatRoughness)
    (coatAnisotropy)
    (coatRotation)
    (coatIOR)
    (coatNormal)
    (coatAffectColor)
    (coatAffectRoughness)
    (thinFilmThickness)
    (thinFilmIOR)
    (emissionColor)
    (thinWalled)

    // Output:
    (outColor)
);
// clang-format on

// Mapping from Maya to MaterialX:
using TokenHashMap = std::unordered_map<TfToken, TfToken, TfToken::HashFunctor>;
TokenHashMap mayaToMaterialX {
    { _tokens->base, _tokens->base },
    { _tokens->baseColor, _tokens->base_color },
    { _tokens->diffuseRoughness, _tokens->diffuse_roughness },
    { _tokens->metalness, _tokens->metalness },
    { _tokens->specular, _tokens->specular },
    { _tokens->specularColor, _tokens->specular_color },
    { _tokens->specularRoughness, _tokens->specular_roughness },
    { _tokens->specularIOR, _tokens->specular_IOR },
    { _tokens->specularAnisotropy, _tokens->specular_anisotropy },
    { _tokens->specularRotation, _tokens->specular_rotation },
    { _tokens->transmission, _tokens->transmission },
    { _tokens->transmissionColor, _tokens->transmission_color },
    { _tokens->transmissionDepth, _tokens->transmission_depth },
    { _tokens->transmissionScatter, _tokens->transmission_scatter },
    { _tokens->transmissionScatterAnisotropy, _tokens->transmission_scatter_anisotropy },
    { _tokens->transmissionDispersion, _tokens->transmission_dispersion },
    { _tokens->transmissionExtraRoughness, _tokens->transmission_extra_roughness },
    { _tokens->subsurface, _tokens->subsurface },
    { _tokens->subsurfaceColor, _tokens->subsurface_color },
    { _tokens->subsurfaceRadius, _tokens->subsurface_radius },
    { _tokens->subsurfaceScale, _tokens->subsurface_scale },
    { _tokens->subsurfaceAnisotropy, _tokens->subsurface_anisotropy },
    { _tokens->sheen, _tokens->sheen },
    { _tokens->sheenColor, _tokens->sheen_color },
    { _tokens->sheenRoughness, _tokens->sheen_roughness },
    { _tokens->coat, _tokens->coat },
    { _tokens->coatColor, _tokens->coat_color },
    { _tokens->coatRoughness, _tokens->coat_roughness },
    { _tokens->coatAnisotropy, _tokens->coat_anisotropy },
    { _tokens->coatRotation, _tokens->coat_rotation },
    { _tokens->coatIOR, _tokens->coat_IOR },
    { _tokens->coatNormal, _tokens->coat_normal },
    { _tokens->coatAffectColor, _tokens->coat_affect_color },
    { _tokens->coatAffectRoughness, _tokens->coat_affect_roughness },
    { _tokens->thinFilmThickness, _tokens->thin_film_thickness },
    { _tokens->thinFilmIOR, _tokens->thin_film_IOR },
    { _tokens->emission, _tokens->emission },
    { _tokens->emissionColor, _tokens->emission_color },
    { _tokens->opacity, _tokens->opacity },
    { _tokens->thinWalled, _tokens->thin_walled },
    { _tokens->normal, _tokens->normal },
    { _tokens->tangent, _tokens->tangent }
};

// This is basically UsdMayaSymmetricShaderWriter with a table for attribute renaming:
class MaterialXTranslators_StandardSurfaceWriter : public MaterialXTranslators_BaseWriter
{
public:
    MaterialXTranslators_StandardSurfaceWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    void Write(const UsdTimeCode& usdTime) override;

    UsdAttribute GetShadingAttributeForMayaAttrName(const TfToken& mayaAttrName) override;

private:
    std::unordered_map<TfToken, MPlug, TfToken::HashFunctor> _inputNameAttrMap;
};

PXRUSDMAYA_REGISTER_SHADER_WRITER(standardSurface, MaterialXTranslators_StandardSurfaceWriter);

MaterialXTranslators_StandardSurfaceWriter::MaterialXTranslators_StandardSurfaceWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : MaterialXTranslators_BaseWriter(depNodeFn, usdPath, jobCtx)
{
    UsdShadeShader shaderSchema = UsdShadeShader::Define(GetUsdStage(), GetUsdPath());
    if (!TF_VERIFY(
            shaderSchema,
            "Could not define UsdShadeShader at path '%s'\n",
            GetUsdPath().GetText())) {
        return;
    }

    _usdPrim = shaderSchema.GetPrim();
    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdShadeShader at path '%s'\n",
            shaderSchema.GetPath().GetText())) {
        return;
    }

    shaderSchema.CreateIdAttr(VtValue(_tokens->ND_standard_surface_surfaceshader));

    UsdShadeNodeGraph nodegraphSchema(GetNodeGraph());
    if (!TF_VERIFY(
            nodegraphSchema,
            "Could not define UsdShadeNodeGraph at path '%s'\n",
            GetUsdPath().GetText())) {
        return;
    }

    for (unsigned int i = 0u; i < depNodeFn.attributeCount(); ++i) {
        const MObject      attrObj = depNodeFn.reorderedAttribute(i);
        const MFnAttribute attrFn(attrObj);

        const TfToken                mayaAttrName = TfToken(attrFn.name().asChar());
        TokenHashMap::const_iterator renaming = mayaToMaterialX.find(mayaAttrName);

        if (renaming == mayaToMaterialX.cend()) {
            continue;
        }

        MPlug attrPlug = depNodeFn.findPlug(attrObj, true);

        const TfToken usdAttrName = renaming->second;

        // Keep our authoring sparse by ignoring attributes with no values set
        // and no connections. We know that the default value of base and base
        // color diverged between Maya and MaterialX in version 1.38.
        if (!(UsdMayaUtil::IsAuthored(attrPlug) || usdAttrName == _tokens->base
              || usdAttrName == _tokens->base_color)
            && !attrPlug.isConnected()) {
            continue;
        }

        const SdfValueTypeName valueTypeName = MayaUsd::Converter::getUsdTypeName(attrPlug);
        if (!valueTypeName) {
            // Unsupported Maya attribute type (e.g. "message" attributes).
            continue;
        }

        // If the Maya attribute is writable, we assume it must be an input.
        // Inputs can still be connected as sources to inputs on other shaders.
        if (attrFn.isWritable()) {
            UsdShadeInput input = shaderSchema.CreateInput(usdAttrName, valueTypeName);
            if (!input) {
                continue;
            }

            if (attrPlug.isElement()) {
                UsdMayaRoundTripUtil::MarkAttributeAsArray(input.GetAttr(), 0u);
            }

            // Add this input to the name/attrPlug map. We'll iterate through
            // these entries during Write() to set their values.
            _inputNameAttrMap.insert(std::make_pair(usdAttrName, attrPlug));

            // All connections go directly to the node graph:
            if (attrPlug.isConnected()) {
                UsdShadeOutput ngOutput = nodegraphSchema.CreateOutput(mayaAttrName, valueTypeName);
                input.ConnectToSource(ngOutput);
            }
        } else if (attrPlug.isConnected()) {
            // Only author outputs for non-writable attributes if they are
            // connected.
            shaderSchema.CreateOutput(usdAttrName, valueTypeName);
        }
    }

    // Surface Output
    shaderSchema.CreateOutput(UsdShadeTokens->surface, SdfValueTypeNames->Token);
}

/* override */
void MaterialXTranslators_StandardSurfaceWriter::Write(const UsdTimeCode& usdTime)
{
    UsdMayaShaderWriter::Write(usdTime);

    UsdShadeShader shaderSchema(_usdPrim);
    if (!TF_VERIFY(
            shaderSchema,
            "Could not get UsdShadeShader schema for UsdPrim at path '%s'\n",
            _usdPrim.GetPath().GetText())) {
        return;
    }

    for (const auto& inputAttrPair : _inputNameAttrMap) {
        const TfToken& inputName = inputAttrPair.first;
        const MPlug&   attrPlug = inputAttrPair.second;

        UsdShadeInput input = shaderSchema.GetInput(inputName);
        if (!input || attrPlug.isConnected()) {
            continue;
        }

        // Color values are all linear on the shader, so do not re-linearize
        // them.
        VtValue value = UsdMayaWriteUtil::GetVtValue(
            attrPlug,
            Converter::getUsdTypeName(attrPlug),
            /* linearizeColors = */ false);

        input.Set(value, usdTime);
    }
}

/* override */
UsdAttribute MaterialXTranslators_StandardSurfaceWriter::GetShadingAttributeForMayaAttrName(
    const TfToken& mayaAttrName)
{
    if (mayaAttrName == _tokens->outColor) {
        UsdShadeShader surfaceSchema(_usdPrim);
        if (!surfaceSchema) {
            return UsdAttribute();
        }

        // Surface output is on the shader itself
        return surfaceSchema.GetOutput(UsdShadeTokens->surface);
    }

    // All other are outputs of the NodeGraph:
    UsdShadeNodeGraph nodegraphSchema(GetNodeGraph());
    if (!nodegraphSchema) {
        return UsdAttribute();
    }

    // And they use the camelCase Maya name directly:
    return nodegraphSchema.GetOutput(mayaAttrName);
}

PXR_NAMESPACE_CLOSE_SCOPE
