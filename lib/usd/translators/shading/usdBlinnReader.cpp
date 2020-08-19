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

#include <maya/MFnBlinnShader.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurface.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Shader writer for importing UsdPreviewSurface to Maya's lambert material nodes
class PxrUsdTranslators_BlinnReader : public PxrUsdTranslators_LambertReader {
    typedef PxrUsdTranslators_LambertReader _BaseClass;

public:
    PxrUsdTranslators_BlinnReader(const UsdMayaPrimReaderArgs&);

    static ContextSupport CanImport(const UsdMayaJobImportArgs& importArgs);

    /// Get the name of the Maya shading attribute that corresponds to the
    /// USD attribute named \p usdAttrName.
    TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const override;

protected:
    /// What is the Maya node type name we want to convert to:
    const TfToken& _GetMayaNodeTypeName() const override;

    void _OnReadAttribute(const TfToken& mayaAttrName, MFnDependencyNode& shaderFn) const override;
};

PXRUSDMAYA_REGISTER_SHADER_READER(UsdPreviewSurface, PxrUsdTranslators_BlinnReader);

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Maya material nodes attribute names
    (eccentricity)
    (specularColor)
);
// clang-format on

PxrUsdTranslators_BlinnReader::PxrUsdTranslators_BlinnReader(const UsdMayaPrimReaderArgs& readArgs)
    : PxrUsdTranslators_LambertReader(readArgs)
{
}

/* static */
UsdMayaShaderReader::ContextSupport
PxrUsdTranslators_BlinnReader::CanImport(const UsdMayaJobImportArgs& importArgs)
{
    return importArgs.shadingConversion == UsdMayaShadingConversionTokens->blinn
        ? ContextSupport::Supported
        : ContextSupport::Unsupported;
}

const TfToken& PxrUsdTranslators_BlinnReader::_GetMayaNodeTypeName() const
{
    return UsdMayaShadingConversionTokens->blinn;
}

void PxrUsdTranslators_BlinnReader::_OnReadAttribute(
    const TfToken&     mayaAttrName,
    MFnDependencyNode& shaderFn) const
{
    if (mayaAttrName == _tokens->specularColor) {
        MFnBlinnShader blinnFn;
        blinnFn.setObject(shaderFn.object());
        MColor color(blinnFn.specularColor());
        float  scale(blinnFn.specularRollOff());
        color /= scale;
        blinnFn.setSpecularColor(color);
        blinnFn.setSpecularRollOff(1.0f);
    } else {
        return _BaseClass::_OnReadAttribute(mayaAttrName, shaderFn);
    }
}

/* virtual */
TfToken PxrUsdTranslators_BlinnReader::GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const
{
    TfToken               usdInputName;
    UsdShadeAttributeType attrType;
    std::tie(usdInputName, attrType) = UsdShadeUtils::GetBaseNameAndType(usdAttrName);

    if (attrType == UsdShadeAttributeType::Input) {
        if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->SpecularColorAttrName) {
            return _tokens->specularColor;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->RoughnessAttrName) {
            return _tokens->eccentricity;
        }
    }

    return _BaseClass::GetMayaNameForUsdAttrName(usdAttrName);
}

PXR_NAMESPACE_CLOSE_SCOPE
