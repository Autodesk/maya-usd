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

#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/tokens.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MStatus.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurface.h>

#include <cmath>

PXR_NAMESPACE_OPEN_SCOPE

class PxrUsdTranslators_BlinnWriter : public PxrUsdTranslators_ReflectWriter
{
    typedef PxrUsdTranslators_ReflectWriter BaseClass;

public:
    PxrUsdTranslators_BlinnWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    void Write(const UsdTimeCode& usdTime) override;

    TfToken GetShadingAttributeNameForMayaAttrName(const TfToken& mayaAttrName) override;

protected:
    void WriteSpecular(const UsdTimeCode& usdTime) override;
};

PXRUSDMAYA_REGISTER_SHADER_WRITER(blinn, PxrUsdTranslators_BlinnWriter);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Maya material nodes attribute names
    (eccentricity)(specularColor)(specularRollOff));

PxrUsdTranslators_BlinnWriter::PxrUsdTranslators_BlinnWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : PxrUsdTranslators_ReflectWriter(depNodeFn, usdPath, jobCtx)
{
}

/* virtual */
void PxrUsdTranslators_BlinnWriter::Write(const UsdTimeCode& usdTime)
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

    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        _tokens->eccentricity,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->RoughnessAttrName,
        usdTime);
}

/* virtual */
void PxrUsdTranslators_BlinnWriter::WriteSpecular(const UsdTimeCode& usdTime)
{
    MStatus                 status;
    const MFnDependencyNode depNodeFn(GetMayaObject(), &status);
    if (status != MS::kSuccess) {
        return;
    }

    UsdShadeShader shaderSchema(_usdPrim);

    AuthorShaderInputFromScaledShadingNodeAttr(
        depNodeFn,
        _tokens->specularColor,
        shaderSchema,
        _tokens->specularColor,
        usdTime,
        _tokens->specularRollOff);

    shaderSchema
        .CreateInput(
            PxrMayaUsdPreviewSurfaceTokens->UseSpecularWorkflowAttrName, SdfValueTypeNames->Int)
        .Set(1, usdTime);

    // Not calling base class. Completely different specular implementation.
}

/* virtual */
TfToken
PxrUsdTranslators_BlinnWriter::GetShadingAttributeNameForMayaAttrName(const TfToken& mayaAttrName)
{
    if (mayaAttrName == _tokens->eccentricity) {
        return UsdShadeUtils::GetFullName(
            PxrMayaUsdPreviewSurfaceTokens->RoughnessAttrName, UsdShadeAttributeType::Input);
    }

    return BaseClass::GetShadingAttributeNameForMayaAttrName(mayaAttrName);
}

PXR_NAMESPACE_CLOSE_SCOPE
