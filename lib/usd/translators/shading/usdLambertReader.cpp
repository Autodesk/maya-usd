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
#include <maya/MFnLambertShader.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurface.h>

PXR_NAMESPACE_OPEN_SCOPE

PXRUSDMAYA_REGISTER_SHADER_READER(UsdPreviewSurface, PxrUsdTranslators_LambertReader);

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Maya material nodes attribute names
    (color)
    (incandescence)
    (normalCamera)
);
// clang-format on

PxrUsdTranslators_LambertReader::PxrUsdTranslators_LambertReader(
    const UsdMayaPrimReaderArgs& readArgs)
    : PxrUsdTranslators_MaterialReader(readArgs)
{
}

/* static */
UsdMayaShaderReader::ContextSupport
PxrUsdTranslators_LambertReader::CanImport(const UsdMayaJobImportArgs& importArgs)
{
    return importArgs.shadingConversion == UsdMayaShadingConversionTokens->lambert
        ? ContextSupport::Supported
        : ContextSupport::Unsupported;
}

const TfToken& PxrUsdTranslators_LambertReader::_GetMayaNodeTypeName() const
{
    return UsdMayaShadingConversionTokens->lambert;
}

void PxrUsdTranslators_LambertReader::_OnReadAttribute(
    const TfToken&     mayaAttrName,
    MFnDependencyNode& shaderFn) const
{
    if (mayaAttrName == _tokens->color) {
        MFnLambertShader lambertFn;
        lambertFn.setObject(shaderFn.object());
        MColor color(lambertFn.color());
        float  scale(lambertFn.diffuseCoeff());
        color /= scale;
        lambertFn.setColor(color);
        lambertFn.setDiffuseCoeff(1.0f);
    } else {
        return _BaseClass::_OnReadAttribute(mayaAttrName, shaderFn);
    }
}

/* virtual */
TfToken PxrUsdTranslators_LambertReader::GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const
{
    TfToken               usdInputName;
    UsdShadeAttributeType attrType;
    std::tie(usdInputName, attrType) = UsdShadeUtils::GetBaseNameAndType(usdAttrName);

    if (attrType == UsdShadeAttributeType::Input) {
        if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->DiffuseColorAttrName) {
            return _tokens->color;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->EmissiveColorAttrName) {
            return _tokens->incandescence;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->NormalAttrName) {
            return _tokens->normalCamera;
        }
    }

    return _BaseClass::GetMayaNameForUsdAttrName(usdAttrName);
}

PXR_NAMESPACE_CLOSE_SCOPE
