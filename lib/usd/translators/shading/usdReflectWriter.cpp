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
#include "usdReflectWriter.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MStatus.h>

#include <pxr/pxr.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/tokens.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Maya material nodes attribute names
    (specularColor)

    // UsdPreviewSurface
    (useSpecularWorkflow)
);

PxrUsdTranslators_ReflectWriter::PxrUsdTranslators_ReflectWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath& usdPath,
        UsdMayaWriteJobContext& jobCtx)
    : PxrUsdTranslators_LambertWriter(depNodeFn, usdPath, jobCtx) {}

/* virtual */
void
PxrUsdTranslators_ReflectWriter::WriteSpecular(const UsdTimeCode& usdTime)
{
    MStatus status;
    const MFnDependencyNode depNodeFn(GetMayaObject(), &status);
    if (status != MS::kSuccess) {
        return;
    }

    UsdShadeShader shaderSchema(_usdPrim);

    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        _tokens->specularColor,
        shaderSchema,
        _tokens->specularColor,
        usdTime);

    shaderSchema.CreateInput(
        _tokens->useSpecularWorkflow,
        SdfValueTypeNames->Int).Set(1, usdTime);

    // Not calling base class since it is not reflective.
}

/* virtual */
TfToken
PxrUsdTranslators_ReflectWriter::GetShadingAttributeNameForMayaAttrName(
        const TfToken& mayaAttrName)
{
    if (!_usdPrim) {
        return TfToken();
    }

    if (mayaAttrName == _tokens->specularColor) {
        return TfToken(
                    TfStringPrintf(
                        "%s%s",
                        UsdShadeTokens->inputs.GetText(),
                        _tokens->specularColor.GetText()).c_str());
    } else {
        return baseClass::GetShadingAttributeNameForMayaAttrName(mayaAttrName);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
