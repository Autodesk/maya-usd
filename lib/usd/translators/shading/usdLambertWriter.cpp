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

#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/utils/util.h>

#include <pxr/pxr.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/tokens.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MStatus.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurface.h>

#include <cmath>

PXR_NAMESPACE_OPEN_SCOPE

PXRUSDMAYA_REGISTER_WRITER(lambert, PxrUsdTranslators_LambertWriter);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Maya material nodes attribute names
    (color)
    (diffuse)
    (incandescence)
    (normalCamera)
);

PxrUsdTranslators_LambertWriter::PxrUsdTranslators_LambertWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : BaseClass(depNodeFn, usdPath, jobCtx)
{
}

/* virtual */
void PxrUsdTranslators_LambertWriter::Write(const UsdTimeCode& usdTime)
{
    BaseClass::Write(usdTime);

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
        PxrMayaUsdPreviewSurfaceTokens->DiffuseColorAttrName,
        usdTime,
        _tokens->diffuse);

    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        _tokens->incandescence,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->EmissiveColorAttrName,
        usdTime);

    // Exported, but unsupported in hdStorm.
    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        _tokens->normalCamera,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->NormalAttrName,
        usdTime);

    WriteSpecular(usdTime);
}

/* virtual */
void PxrUsdTranslators_LambertWriter::WriteSpecular(const UsdTimeCode& usdTime)
{
    // No specular on plain Lambert
    UsdShadeShader shaderSchema(_usdPrim);

    shaderSchema
        .CreateInput(PxrMayaUsdPreviewSurfaceTokens->RoughnessAttrName, SdfValueTypeNames->Float)
        .Set(1.0f, usdTime);

    // Using specular workflow, but enforced black specular color.
    shaderSchema
        .CreateInput(
            PxrMayaUsdPreviewSurfaceTokens->UseSpecularWorkflowAttrName, SdfValueTypeNames->Int)
        .Set(1, usdTime);

    shaderSchema
        .CreateInput(PxrMayaUsdPreviewSurfaceTokens->SpecularColorAttrName, SdfValueTypeNames->Color3f)
        .Set(GfVec3f(0.0f, 0.0f, 0.0f), usdTime);

}

/* virtual */
TfToken
PxrUsdTranslators_LambertWriter::GetShadingAttributeNameForMayaAttrName(const TfToken& mayaAttrName)
{
    TfToken usdAttrName;

    if (mayaAttrName == _tokens->color) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->DiffuseColorAttrName;
    } else if (mayaAttrName == _tokens->incandescence) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->EmissiveColorAttrName;
    } else if (mayaAttrName == _tokens->normalCamera) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->NormalAttrName;
    } else {
        return BaseClass::GetShadingAttributeNameForMayaAttrName(mayaAttrName);
    }

    return UsdShadeUtils::GetFullName(usdAttrName, UsdShadeAttributeType::Input);
}

PXR_NAMESPACE_CLOSE_SCOPE
