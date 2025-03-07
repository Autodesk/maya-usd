//
// Copyright 2024 Autodesk
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
#include <maya/MFnOpenPBRSurfaceShader.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurface.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Shader reader for importing UsdPreviewSurface to Maya's openPBRShader material nodes
class PxrUsdTranslators_OpenPBRSurfaceReader : public PxrUsdTranslators_MaterialReader
{
    using _BaseClass = PxrUsdTranslators_MaterialReader;

public:
    PxrUsdTranslators_OpenPBRSurfaceReader(const UsdMayaPrimReaderArgs&);

    static ContextSupport CanImport(const UsdMayaJobImportArgs& importArgs);

    /// Get the name of the Maya shading attribute that corresponds to the
    /// USD attribute named \p usdAttrName.
    TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const override;

protected:
    /// What is the Maya node type name we want to convert to:
    const TfToken& _GetMayaNodeTypeName() const override;

    /// Callback called before the attribute \p mayaAttribute is read from UsdShade. This allows
    /// setting back values in \p shaderFn that were lost during the export phase.
    void
    _OnBeforeReadAttribute(const TfToken& mayaAttrName, MFnDependencyNode& shaderFn) const override;
};

PXRUSDMAYA_REGISTER_SHADER_READER(UsdPreviewSurface, PxrUsdTranslators_OpenPBRSurfaceReader);

PxrUsdTranslators_OpenPBRSurfaceReader::PxrUsdTranslators_OpenPBRSurfaceReader(
    const UsdMayaPrimReaderArgs& readArgs)
    : PxrUsdTranslators_MaterialReader(readArgs)
{
}

/* static */
UsdMayaShaderReader::ContextSupport
PxrUsdTranslators_OpenPBRSurfaceReader::CanImport(const UsdMayaJobImportArgs& importArgs)
{
    // Check to see if import requested conversion
    return importArgs.preferredMaterial == UsdMayaPreferredMaterialTokens->openPBRSurface
        ? ContextSupport::Supported
        : ContextSupport::Unsupported;
}

const TfToken& PxrUsdTranslators_OpenPBRSurfaceReader::_GetMayaNodeTypeName() const
{
    return UsdMayaPreferredMaterialTokens->openPBRSurface;
}

void PxrUsdTranslators_OpenPBRSurfaceReader::_OnBeforeReadAttribute(
    const TfToken&     mayaAttrName,
    MFnDependencyNode& shaderFn) const
{
    MFnOpenPBRSurfaceShader surfaceFn;
    surfaceFn.setObject(shaderFn.object());
    if (mayaAttrName == TrMayaOpenPBRTokens->baseColor) {
        MColor color(surfaceFn.baseColor());
        float  scale(surfaceFn.baseWeight());
        if (scale != 0.0f) {
            color /= scale;
        }
        surfaceFn.setBaseColor(color);
        surfaceFn.setBaseWeight(1.0f);
    } else if (mayaAttrName == TrMayaOpenPBRTokens->emissionColor) {
#ifdef MAYA_OPENPBR_HAS_EMISSION_WEIGHT
        MColor color(surfaceFn.emissionColor());
        float  luminance(surfaceFn.emissionLuminance());
        float  scale(surfaceFn.emissionWeight());
        if (scale != 0.0f) {
            color /= scale;
        }
        if (luminance != 0.0f) {
            color /= (luminance / 1000.0F);
        }
        surfaceFn.setEmissionColor(color);
        surfaceFn.setEmissionLuminance(1000.0f);
        surfaceFn.setEmissionWeight(1.0f);
#else
        MColor color(surfaceFn.emissionColor());
        float  scale(surfaceFn.emissionLuminance());
        if (scale != 0.0f) {
            color /= (scale / 1000.0f);
        }
        surfaceFn.setEmissionColor(color);
        surfaceFn.setEmissionLuminance(1000.0f);
#endif
    } else {
        _BaseClass::_OnBeforeReadAttribute(mayaAttrName, shaderFn);
    }
}

/* virtual */
TfToken
PxrUsdTranslators_OpenPBRSurfaceReader::GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const
{
    TfToken               usdInputName;
    UsdShadeAttributeType attrType;
    std::tie(usdInputName, attrType) = UsdShadeUtils::GetBaseNameAndType(usdAttrName);

    if (attrType == UsdShadeAttributeType::Input) {
        if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->DiffuseColorAttrName) {
            return TrMayaOpenPBRTokens->baseColor;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->EmissiveColorAttrName) {
            return TrMayaOpenPBRTokens->emissionColor;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->MetallicAttrName) {
            return TrMayaOpenPBRTokens->baseMetalness;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->SpecularColorAttrName) {
            return TrMayaOpenPBRTokens->specularColor;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->IorAttrName) {
            return TrMayaOpenPBRTokens->specularIOR;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->RoughnessAttrName) {
            return TrMayaOpenPBRTokens->specularRoughness;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->ClearcoatRoughnessAttrName) {
            return TrMayaOpenPBRTokens->coatRoughness;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->NormalAttrName) {
            return TrMayaOpenPBRTokens->normalCamera;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->ClearcoatAttrName) {
            return TrMayaOpenPBRTokens->coatWeight;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->OpacityAttrName) {
            return TrMayaOpenPBRTokens->geometryOpacity;
        }
    }

    return _BaseClass::GetMayaNameForUsdAttrName(usdAttrName);
}

PXR_NAMESPACE_CLOSE_SCOPE
