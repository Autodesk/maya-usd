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
#include "shadingTokens.h"
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

/// Shader reader for importing UsdPreviewSurface to Maya's standardSurface material nodes
class PxrUsdTranslators_StandardSurfaceReader : public PxrUsdTranslators_MaterialReader
{
    using _BaseClass = PxrUsdTranslators_MaterialReader;

public:
    PxrUsdTranslators_StandardSurfaceReader(const UsdMayaPrimReaderArgs&);

    static ContextSupport CanImport(const UsdMayaJobImportArgs& importArgs);

    /// Get the name of the Maya shading attribute that corresponds to the
    /// USD attribute named \p usdAttrName.
    TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const override;

protected:
    /// What is the Maya node type name we want to convert to:
    const TfToken& _GetMayaNodeTypeName() const override;

    /// Convert the value in \p usdValue from USD back to Maya following rules
    /// for attribute \p mayaAttrName
    void _ConvertToMaya(const TfToken& mayaAttrName, VtValue& usdValue) const override;

    /// Callback called before the attribute \p mayaAttribute is read from UsdShade. This allows
    /// setting back values in \p shaderFn that were lost during the export phase.
    void
    _OnBeforeReadAttribute(const TfToken& mayaAttrName, MFnDependencyNode& shaderFn) const override;
};

PXRUSDMAYA_REGISTER_SHADER_READER(UsdPreviewSurface, PxrUsdTranslators_StandardSurfaceReader);

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
    return importArgs.preferredMaterial == UsdMayaPreferredMaterialTokens->standardSurface
        ? ContextSupport::Supported
        : ContextSupport::Unsupported;
}

const TfToken& PxrUsdTranslators_StandardSurfaceReader::_GetMayaNodeTypeName() const
{
    return UsdMayaPreferredMaterialTokens->standardSurface;
}

void PxrUsdTranslators_StandardSurfaceReader::_ConvertToMaya(
    const TfToken& mayaAttrName,
    VtValue&       usdValue) const
{
    if (mayaAttrName == TrMayaTokens->transmission && usdValue.IsHolding<float>()) {
        usdValue = 1.0f - usdValue.UncheckedGet<float>();
    }
}

void PxrUsdTranslators_StandardSurfaceReader::_OnBeforeReadAttribute(
    const TfToken&     mayaAttrName,
    MFnDependencyNode& shaderFn) const
{
    MFnStandardSurfaceShader surfaceFn;
    surfaceFn.setObject(shaderFn.object());
    if (mayaAttrName == TrMayaTokens->baseColor) {
        MColor color(surfaceFn.baseColor());
        float  scale(surfaceFn.base());
        color /= scale;
        surfaceFn.setBaseColor(color);
        surfaceFn.setBase(1.0f);
    } else if (mayaAttrName == TrMayaTokens->emissionColor) {
        MColor color(surfaceFn.emissionColor());
        float  scale(surfaceFn.emission());
        color /= scale;
        surfaceFn.setEmissionColor(color);
        surfaceFn.setEmission(1.0f);
    } else {
        _BaseClass::_OnBeforeReadAttribute(mayaAttrName, shaderFn);
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
            return TrMayaTokens->baseColor;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->EmissiveColorAttrName) {
            return TrMayaTokens->emissionColor;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->MetallicAttrName) {
            return TrMayaTokens->metalness;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->SpecularColorAttrName) {
            return TrMayaTokens->specularColor;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->IorAttrName) {
            return TrMayaTokens->specularIOR;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->RoughnessAttrName) {
            return TrMayaTokens->specularRoughness;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->ClearcoatRoughnessAttrName) {
            return TrMayaTokens->coatRoughness;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->NormalAttrName) {
            return TrMayaTokens->normalCamera;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->ClearcoatAttrName) {
            return TrMayaTokens->coat;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->OpacityAttrName) {
            return TrMayaTokens->transmission;
        }
    }

    return _BaseClass::GetMayaNameForUsdAttrName(usdAttrName);
}

PXR_NAMESPACE_CLOSE_SCOPE
