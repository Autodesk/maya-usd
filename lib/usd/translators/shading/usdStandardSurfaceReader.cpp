//
// Copyright 2020 Autodesk
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
#include "usdMaterialReader.h"

#include <mayaUsd/fileio/shaderReader.h>
#include <mayaUsd/fileio/shaderReaderRegistry.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>
#include <mayaUsd/fileio/utils/readUtil.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usdShade/input.h>
#include <pxr/usd/usdShade/output.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/tokens.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MFnStandardSurfaceShader.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurface.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Shader writer for importing UsdPreviewSurface to Maya's lambert material nodes
class PxrUsdTranslators_StandardSurfaceReader : public PxrUsdTranslators_MaterialReader {
    typedef PxrUsdTranslators_MaterialReader _BaseClass;

public:
    PxrUsdTranslators_StandardSurfaceReader(const UsdMayaPrimReaderArgs&);

    static ContextSupport CanImport(const UsdMayaJobImportArgs& importArgs);

    /// Get the name of the Maya shading attribute that corresponds to the
    /// USD attribute named \p usdAttrName.
    TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const override;

protected:
    const TfToken& _GetMayaNodeTypeName() const override;
    void           _ConvertToMaya(const TfToken& mayaAttrName, VtValue& usdValue) const override;
    void _OnReadAttribute(const TfToken& mayaAttrName, MFnDependencyNode& shaderFn) const override;
};

PXRUSDMAYA_REGISTER_SHADER_READER(UsdPreviewSurface, PxrUsdTranslators_StandardSurfaceReader);

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Maya material nodes attribute names
    (base)
    (baseColor)
    (emission)
    (emissionColor)
    (metalness)
    (specularColor)
    (specularIOR)
    (specularRoughness)
    (coat)
    (coatRoughness)
    (transmission)
    (normalCamera)
);
// clang-format on

PxrUsdTranslators_StandardSurfaceReader::PxrUsdTranslators_StandardSurfaceReader(
    const UsdMayaPrimReaderArgs& readArgs)
    : PxrUsdTranslators_MaterialReader(readArgs)
{
}

/* static */
UsdMayaShaderReader::ContextSupport
PxrUsdTranslators_StandardSurfaceReader::CanImport(const UsdMayaJobImportArgs& importArgs)
{
    // Check to see if import requested conversion
    return importArgs.shadingConversion == UsdMayaShadingConversionTokens->standardSurface
        ? ContextSupport::Supported
        : ContextSupport::Unsupported;
}

const TfToken& PxrUsdTranslators_StandardSurfaceReader::_GetMayaNodeTypeName() const
{
    return UsdMayaShadingConversionTokens->standardSurface;
}

void PxrUsdTranslators_StandardSurfaceReader::_ConvertToMaya(
    const TfToken& mayaAttrName,
    VtValue&       usdValue) const
{
    if (mayaAttrName == _tokens->transmission && usdValue.IsHolding<float>()) {
        usdValue = 1.0f - usdValue.UncheckedGet<float>();
    }
}

void PxrUsdTranslators_StandardSurfaceReader::_OnReadAttribute(
    const TfToken&     mayaAttrName,
    MFnDependencyNode& shaderFn) const
{
    MFnStandardSurfaceShader surfaceFn;
    surfaceFn.setObject(shaderFn.object());
    if (mayaAttrName == _tokens->baseColor) {
        MColor color(surfaceFn.baseColor());
        float  scale(surfaceFn.base());
        color /= scale;
        surfaceFn.setBaseColor(color);
        surfaceFn.setBase(1.0f);
    } else if (mayaAttrName == _tokens->emissionColor) {
        MColor color(surfaceFn.emissionColor());
        float  scale(surfaceFn.emission());
        color /= scale;
        surfaceFn.setEmissionColor(color);
        surfaceFn.setEmission(1.0f);
    } else {
        return _BaseClass::_OnReadAttribute(mayaAttrName, shaderFn);
    }
}

/* virtual */
TfToken
PxrUsdTranslators_StandardSurfaceReader::GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const
{
    TfToken               usdInputName;
    UsdShadeAttributeType attrType;
    std::tie(usdInputName, attrType) = UsdShadeUtils::GetBaseNameAndType(usdAttrName);

    if (attrType == UsdShadeAttributeType::Input) {
        if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->DiffuseColorAttrName) {
            return _tokens->baseColor;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->EmissiveColorAttrName) {
            return _tokens->emissionColor;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->MetallicAttrName) {
            return _tokens->metalness;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->SpecularColorAttrName) {
            return _tokens->specularColor;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->IorAttrName) {
            return _tokens->specularIOR;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->RoughnessAttrName) {
            return _tokens->specularRoughness;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->ClearcoatRoughnessAttrName) {
            return _tokens->coatRoughness;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->NormalAttrName) {
            return _tokens->normalCamera;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->ClearcoatAttrName) {
            return _tokens->coat;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->OpacityAttrName) {
            return _tokens->transmission;
        }
    }

    return _BaseClass::GetMayaNameForUsdAttrName(usdAttrName);
}

PXR_NAMESPACE_CLOSE_SCOPE
