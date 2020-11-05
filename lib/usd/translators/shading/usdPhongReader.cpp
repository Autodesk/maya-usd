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
#include "usdLambertReader.h"

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
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurface.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Shader reader for importing UsdPreviewSurface to Maya's phong material nodes
class PxrUsdTranslators_PhongReader : public PxrUsdTranslators_LambertReader
{
    using _BaseClass = PxrUsdTranslators_LambertReader;

public:
    PxrUsdTranslators_PhongReader(const UsdMayaPrimReaderArgs&);

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
};

PXRUSDMAYA_REGISTER_SHADER_READER(UsdPreviewSurface, PxrUsdTranslators_PhongReader);

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Maya material nodes attribute names
    (cosinePower)
    (specularColor)
);
// clang-format on

PxrUsdTranslators_PhongReader::PxrUsdTranslators_PhongReader(const UsdMayaPrimReaderArgs& readArgs)
    : PxrUsdTranslators_LambertReader(readArgs)
{
}

/* static */
UsdMayaShaderReader::ContextSupport
PxrUsdTranslators_PhongReader::CanImport(const UsdMayaJobImportArgs& importArgs)
{
    return importArgs.preferredMaterial == UsdMayaPreferredMaterialTokens->phong
        ? ContextSupport::Supported
        : ContextSupport::Unsupported;
}

const TfToken& PxrUsdTranslators_PhongReader::_GetMayaNodeTypeName() const
{
    return UsdMayaPreferredMaterialTokens->phong;
}

void PxrUsdTranslators_PhongReader::_ConvertToMaya(const TfToken& mayaAttrName, VtValue& usdValue)
    const
{
    if (mayaAttrName == _tokens->cosinePower && usdValue.IsHolding<float>()) {
        float roughness = usdValue.UncheckedGet<float>();
        float squared = roughness * roughness;
        // In the maya UI, cosinePower goes from 2.0 to 100.0
        // this does not map directly to specular roughness
        usdValue = GfClamp((1.0f - 3.357f * squared) / (0.454f * squared), 2.0f, 100.0f);
    }
}

/* virtual */
TfToken PxrUsdTranslators_PhongReader::GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const
{
    TfToken               usdInputName;
    UsdShadeAttributeType attrType;
    std::tie(usdInputName, attrType) = UsdShadeUtils::GetBaseNameAndType(usdAttrName);

    if (attrType == UsdShadeAttributeType::Input) {
        if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->SpecularColorAttrName) {
            return _tokens->specularColor;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->RoughnessAttrName) {
            return _tokens->cosinePower;
        }
    }

    return _BaseClass::GetMayaNameForUsdAttrName(usdAttrName);
}

PXR_NAMESPACE_CLOSE_SCOPE
