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
#include "shadingTokens.h"

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

namespace {
// Mapping from Maya to MaterialX:
using TokenHashMap = std::unordered_map<TfToken, TfToken, TfToken::HashFunctor>;
TokenHashMap mayaToMaterialX {
    { TrMayaTokens->base, TrMtlxTokens->base },
    { TrMayaTokens->baseColor, TrMtlxTokens->base_color },
    { TrMayaTokens->diffuseRoughness, TrMtlxTokens->diffuse_roughness },
    { TrMayaTokens->metalness, TrMtlxTokens->metalness },
    { TrMayaTokens->specular, TrMtlxTokens->specular },
    { TrMayaTokens->specularColor, TrMtlxTokens->specular_color },
    { TrMayaTokens->specularRoughness, TrMtlxTokens->specular_roughness },
    { TrMayaTokens->specularIOR, TrMtlxTokens->specular_IOR },
    { TrMayaTokens->specularAnisotropy, TrMtlxTokens->specular_anisotropy },
    { TrMayaTokens->specularRotation, TrMtlxTokens->specular_rotation },
    { TrMayaTokens->transmission, TrMtlxTokens->transmission },
    { TrMayaTokens->transmissionColor, TrMtlxTokens->transmission_color },
    { TrMayaTokens->transmissionDepth, TrMtlxTokens->transmission_depth },
    { TrMayaTokens->transmissionScatter, TrMtlxTokens->transmission_scatter },
    { TrMayaTokens->transmissionScatterAnisotropy, TrMtlxTokens->transmission_scatter_anisotropy },
    { TrMayaTokens->transmissionDispersion, TrMtlxTokens->transmission_dispersion },
    { TrMayaTokens->transmissionExtraRoughness, TrMtlxTokens->transmission_extra_roughness },
    { TrMayaTokens->subsurface, TrMtlxTokens->subsurface },
    { TrMayaTokens->subsurfaceColor, TrMtlxTokens->subsurface_color },
    { TrMayaTokens->subsurfaceRadius, TrMtlxTokens->subsurface_radius },
    { TrMayaTokens->subsurfaceScale, TrMtlxTokens->subsurface_scale },
    { TrMayaTokens->subsurfaceAnisotropy, TrMtlxTokens->subsurface_anisotropy },
    { TrMayaTokens->sheen, TrMtlxTokens->sheen },
    { TrMayaTokens->sheenColor, TrMtlxTokens->sheen_color },
    { TrMayaTokens->sheenRoughness, TrMtlxTokens->sheen_roughness },
    { TrMayaTokens->coat, TrMtlxTokens->coat },
    { TrMayaTokens->coatColor, TrMtlxTokens->coat_color },
    { TrMayaTokens->coatRoughness, TrMtlxTokens->coat_roughness },
    { TrMayaTokens->coatAnisotropy, TrMtlxTokens->coat_anisotropy },
    { TrMayaTokens->coatRotation, TrMtlxTokens->coat_rotation },
    { TrMayaTokens->coatIOR, TrMtlxTokens->coat_IOR },
    { TrMayaTokens->coatNormal, TrMtlxTokens->coat_normal },
    { TrMayaTokens->coatAffectColor, TrMtlxTokens->coat_affect_color },
    { TrMayaTokens->coatAffectRoughness, TrMtlxTokens->coat_affect_roughness },
    { TrMayaTokens->thinFilmThickness, TrMtlxTokens->thin_film_thickness },
    { TrMayaTokens->thinFilmIOR, TrMtlxTokens->thin_film_IOR },
    { TrMayaTokens->emission, TrMtlxTokens->emission },
    { TrMayaTokens->emissionColor, TrMtlxTokens->emission_color },
    { TrMayaTokens->opacity, TrMtlxTokens->opacity },
    { TrMayaTokens->thinWalled, TrMtlxTokens->thin_walled },
    { TrMayaTokens->normalCamera, TrMtlxTokens->normal },
    { TrMayaTokens->tangentUCamera, TrMtlxTokens->tangent }
};

} // namespace
// This is basically UsdMayaSymmetricShaderWriter with a table for attribute renaming:
class MaterialXTranslators_StandardSurfaceWriter : public MtlxUsd_BaseWriter
{
public:
    MaterialXTranslators_StandardSurfaceWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    void Write(const UsdTimeCode& usdTime) override;

    UsdAttribute
    GetShadingAttributeForMayaAttrName(const TfToken&, const SdfValueTypeName&) override;

private:
    std::unordered_map<TfToken, MPlug, TfToken::HashFunctor> _inputNameAttrMap;
};

PXRUSDMAYA_REGISTER_SHADER_WRITER(standardSurface, MaterialXTranslators_StandardSurfaceWriter);

MaterialXTranslators_StandardSurfaceWriter::MaterialXTranslators_StandardSurfaceWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : MtlxUsd_BaseWriter(depNodeFn, usdPath, jobCtx)
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

    shaderSchema.CreateIdAttr(VtValue(TrMtlxTokens->ND_standard_surface_surfaceshader));

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

        // Keep our authoring sparse by ignoring attributes with no values set and no connections.
        // Some attributes with an history of default value updates will be written always.
        if (!(UsdMayaUtil::IsAuthored(attrPlug) || usdAttrName == TrMtlxTokens->base
              || usdAttrName == TrMtlxTokens->base_color || usdAttrName == TrMtlxTokens->specular
              || usdAttrName == TrMtlxTokens->specular_roughness)
            && !attrPlug.isConnected()) {
            continue;
        }

        const SdfValueTypeName valueTypeName = MayaUsd::Converter::getUsdTypeName(attrPlug);
        if (!valueTypeName) {
            // Unsupported Maya attribute type (e.g. "message" attributes).
            continue;
        }

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
    const TfToken& mayaAttrName,
    const SdfValueTypeName&)
{
    if (mayaAttrName == TrMayaTokens->outColor) {
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

    if (mayaAttrName == TrMayaTokens->normalCamera || mayaAttrName == TrMayaTokens->coatNormal) {
        // Add the proper nodes for normal mapping:
        return AddNormalMapping(nodegraphSchema.GetOutput(mayaAttrName));
    }

    // And they use the camelCase Maya name directly:
    return nodegraphSchema.GetOutput(mayaAttrName);
}

PXR_NAMESPACE_CLOSE_SCOPE
