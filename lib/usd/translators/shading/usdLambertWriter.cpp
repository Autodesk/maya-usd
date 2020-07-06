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
#include "usdLambertWriter.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MStatus.h>

#include <pxr/pxr.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/tokens.h>

#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/utils/util.h>

#include <cmath>

PXR_NAMESPACE_OPEN_SCOPE

PXRUSDMAYA_REGISTER_WRITER(
    lambert,
    PxrUsdTranslators_LambertWriter);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Maya material nodes attribute names
    (color)
    (diffuse)
    (incandescence)
    (normalCamera)

    // UsdPreviewSurface
    (diffuseColor)
    (emissiveColor)
    (normal)
    (roughness)
    (useSpecularWorkflow)
);

PxrUsdTranslators_LambertWriter::PxrUsdTranslators_LambertWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath& usdPath,
        UsdMayaWriteJobContext& jobCtx)
    : baseClass(depNodeFn, usdPath, jobCtx) {}

/* virtual */
void
PxrUsdTranslators_LambertWriter::Write(const UsdTimeCode& usdTime)
{
    baseClass::Write(usdTime);

    MStatus status;

    const MFnDependencyNode depNodeFn(GetMayaObject(), &status);
    if (status != MS::kSuccess) {
        return;
    }

    UsdShadeShader shaderSchema(_usdPrim);
    if (!TF_VERIFY(
            shaderSchema,
            "Could not get UsdShadeShader schema for UsdPrim at path '%s'\n",
            _usdPrim.GetPath().GetText())) {
        return;
    }

    AuthorShaderInputFromScaledShadingNodeAttr(
        depNodeFn,
        _tokens->color,
        shaderSchema,
        _tokens->diffuseColor,
        usdTime,
        _tokens->diffuse);

    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        _tokens->incandescence,
        shaderSchema,
        _tokens->emissiveColor,
        usdTime);

    // Exported, but unsupported in hdStorm.
    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        _tokens->normalCamera,
        shaderSchema,
        _tokens->normal,
        usdTime);

    WriteSpecular(usdTime);
}

/* virtual */
void
PxrUsdTranslators_LambertWriter::WriteSpecular(const UsdTimeCode& usdTime)
{
    // No specular on plain Lambert
    UsdShadeShader shaderSchema(_usdPrim);

    shaderSchema.CreateInput(
        _tokens->roughness,
        SdfValueTypeNames->Float).Set(1.0f, usdTime);

    shaderSchema.CreateInput(
        _tokens->useSpecularWorkflow,
        SdfValueTypeNames->Int).Set(0, usdTime);
}

/* virtual */
TfToken
PxrUsdTranslators_LambertWriter::GetShadingAttributeNameForMayaAttrName(
        const TfToken& mayaAttrName)
{
    if (!_usdPrim) {
        return TfToken();
    }

    TfToken usdPortName;

    if (mayaAttrName == _tokens->color) {
        usdPortName =_tokens->diffuseColor;
    } else if (mayaAttrName == _tokens->incandescence) {
        usdPortName =_tokens->emissiveColor;
    } else if (mayaAttrName == _tokens->normalCamera) {
        usdPortName =_tokens->normal;
    } else {
        return baseClass::GetShadingAttributeNameForMayaAttrName(mayaAttrName);
    }

    return TfToken(
                TfStringPrintf(
                    "%s%s",
                    UsdShadeTokens->inputs.GetText(),
                    usdPortName.GetText()).c_str());
}

PXR_NAMESPACE_CLOSE_SCOPE
