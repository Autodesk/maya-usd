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

#include "shadingTokens.h"

#include <mayaUsd/fileio/shaderReader.h>
#include <mayaUsd/fileio/shaderReaderRegistry.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>
#include <mayaUsd/fileio/utils/readUtil.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/gf/vec3f.h>
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

PxrUsdTranslators_LambertReader::PxrUsdTranslators_LambertReader(
    const UsdMayaPrimReaderArgs& readArgs)
    : PxrUsdTranslators_MaterialReader(readArgs)
{
}

/* static */
UsdMayaShaderReader::ContextSupport
PxrUsdTranslators_LambertReader::CanImport(const UsdMayaJobImportArgs& importArgs)
{
    return importArgs.preferredMaterial == UsdMayaPreferredMaterialTokens->lambert
        ? ContextSupport::Supported
        : ContextSupport::Unsupported;
}

const TfToken& PxrUsdTranslators_LambertReader::_GetMayaNodeTypeName() const
{
    return UsdMayaPreferredMaterialTokens->lambert;
}

void PxrUsdTranslators_LambertReader::_OnBeforeReadAttribute(
    const TfToken&     mayaAttrName,
    MFnDependencyNode& shaderFn) const
{
    if (mayaAttrName == TrMayaTokens->color) {
        MFnLambertShader lambertFn;
        lambertFn.setObject(shaderFn.object());
        MColor color(lambertFn.color());
        float  scale(lambertFn.diffuseCoeff());
        color /= scale;
        lambertFn.setColor(color);
        lambertFn.setDiffuseCoeff(1.0f);
    } else {
        _BaseClass::_OnBeforeReadAttribute(mayaAttrName, shaderFn);
    }
}

/* override */
void PxrUsdTranslators_LambertReader::_ConvertToMaya(const TfToken& mayaAttrName, VtValue& usdValue)
    const
{
    if (mayaAttrName == TrMayaTokens->transparency && usdValue.IsHolding<float>()) {
        const float opacity = usdValue.UncheckedGet<float>();
        usdValue = GfVec3f(1.0f - opacity);
        return;
    }

    PxrUsdTranslators_MaterialReader::_ConvertToMaya(mayaAttrName, usdValue);
}

/* virtual */
TfToken PxrUsdTranslators_LambertReader::GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const
{
    TfToken               usdInputName;
    UsdShadeAttributeType attrType;
    std::tie(usdInputName, attrType) = UsdShadeUtils::GetBaseNameAndType(usdAttrName);

    if (attrType == UsdShadeAttributeType::Input) {
        if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->DiffuseColorAttrName) {
            return TrMayaTokens->color;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->OpacityAttrName) {
            return TrMayaTokens->transparency;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->EmissiveColorAttrName) {
            return TrMayaTokens->incandescence;
        } else if (usdInputName == PxrMayaUsdPreviewSurfaceTokens->NormalAttrName) {
            return TrMayaTokens->normalCamera;
        }
    }

    return _BaseClass::GetMayaNameForUsdAttrName(usdAttrName);
}

PXR_NAMESPACE_CLOSE_SCOPE
