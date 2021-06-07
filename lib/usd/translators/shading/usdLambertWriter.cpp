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

#include "shadingTokens.h"

#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/tokens.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurface.h>

#include <cmath>

PXR_NAMESPACE_OPEN_SCOPE

PXRUSDMAYA_REGISTER_SHADER_WRITER(lambert, PxrUsdTranslators_LambertWriter);

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
        TrMayaTokens->color,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->DiffuseColorAttrName,
        usdTime,
        TrMayaTokens->diffuse);

    const MPlug transparencyPlug = depNodeFn.findPlug(
        depNodeFn.attribute(TrMayaTokens->transparency.GetText()),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status == MS::kSuccess && UsdMayaUtil::IsAuthored(transparencyPlug)) {
        UsdShadeInput opacityInput = shaderSchema.CreateInput(
            PxrMayaUsdPreviewSurfaceTokens->OpacityAttrName, SdfValueTypeNames->Float);

        // For attributes that are the destination of a connection, we create
        // the input on the shader but we do *not* author a value for it. We
        // expect its actual value to come from the source of its connection.
        // We'll leave it to the shading export to handle creating the
        // connections in USD.
        if (!transparencyPlug.isDestination(&status)) {
            const float transparencyAvg
                = (transparencyPlug.child(0u).asFloat() + transparencyPlug.child(1u).asFloat()
                   + transparencyPlug.child(2u).asFloat())
                / 3.0f;

            opacityInput.Set(1.0f - transparencyAvg, usdTime);
        }
    }

    // Since incandescence in Maya and emissiveColor in UsdPreviewSurface are
    // both black by default, only author it in USD if it is authored in Maya.
    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        TrMayaTokens->incandescence,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->EmissiveColorAttrName,
        usdTime,
        /* ignoreIfUnauthored = */ true);

    // Exported, but unsupported in hdStorm.
    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        TrMayaTokens->normalCamera,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->NormalAttrName,
        usdTime,
        /* ignoreIfUnauthored = */ false,
        /* inputTypeName = */ SdfValueTypeNames->Normal3f);

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

    // Using specular workflow. There is no need to author the specular color
    // since UsdPreviewSurface uses black as a fallback value.
    shaderSchema
        .CreateInput(
            PxrMayaUsdPreviewSurfaceTokens->UseSpecularWorkflowAttrName, SdfValueTypeNames->Int)
        .Set(1, usdTime);
}

/* virtual */
TfToken
PxrUsdTranslators_LambertWriter::GetShadingAttributeNameForMayaAttrName(const TfToken& mayaAttrName)
{
    TfToken usdAttrName;

    if (mayaAttrName == TrMayaTokens->color) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->DiffuseColorAttrName;
    } else if (mayaAttrName == TrMayaTokens->transparency) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->OpacityAttrName;
    } else if (mayaAttrName == TrMayaTokens->incandescence) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->EmissiveColorAttrName;
    } else if (mayaAttrName == TrMayaTokens->normalCamera) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->NormalAttrName;
    } else {
        return BaseClass::GetShadingAttributeNameForMayaAttrName(mayaAttrName);
    }

    return UsdShadeUtils::GetFullName(usdAttrName, UsdShadeAttributeType::Input);
}

PXR_NAMESPACE_CLOSE_SCOPE
