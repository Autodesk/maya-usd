//
// Copyright 2016 Pixar
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
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/translators/translatorMaterial.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>
#include <mayaUsd/fileio/utils/readUtil.h>
#include <mayaUsd/utils/colorSpace.h>

#include <pxr/base/gf/gamma.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/base/vt/types.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/gprim.h>
#include <pxr/usd/usdGeom/primvar.h>
#include <pxr/usd/usdRi/materialAPI.h>
#include <pxr/usd/usdShade/connectableAPI.h>
#include <pxr/usd/usdShade/input.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/output.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/tokens.h>

#include <maya/MColor.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnLambertShader.h>
#include <maya/MFnSet.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#if MAYA_API_VERSION >= 20200000
#include <maya/MFnStandardSurfaceShader.h>
#endif

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (diffuseColor)
    (opacity)

    ((NiceName, "Display Colors"))
    ((ImportDescription,
        "Imports the displayColor primvar on the USD mesh as a Maya"
        " surface shader."))
);
// clang-format on

DEFINE_SHADING_MODE_IMPORTER_WITH_JOB_ARGUMENTS(
    displayColor,
    _tokens->NiceName.GetString(),
    _tokens->ImportDescription.GetString(),
    context,
    jobArguments)
{
    const UsdShadeMaterial& shadeMaterial = context->GetShadeMaterial();
    const UsdGeomGprim&     primSchema = context->GetBoundPrim();

    MStatus status;

    // Get Display Color from USD (linear) and convert to Display
    GfVec3f linearDisplayColor(.5, .5, .5);
    GfVec3f linearTransparency(0, 0, 0);

    VtVec3fArray gprimDisplayColor(1);
    if (primSchema && primSchema.GetDisplayColorPrimvar().ComputeFlattened(&gprimDisplayColor)) {
        linearDisplayColor = gprimDisplayColor[0];
        VtFloatArray gprimDisplayOpacity(1);
        if (primSchema.GetDisplayOpacityPrimvar().GetAttr().HasAuthoredValue()
            && primSchema.GetDisplayOpacityPrimvar().ComputeFlattened(&gprimDisplayOpacity)) {
            const float trans = 1.0 - gprimDisplayOpacity[0];
            linearTransparency = GfVec3f(trans, trans, trans);
        }
    } else {
        return MObject();
    }

    const GfVec3f displayColor = MayaUsd::utils::ConvertLinearToMaya(linearDisplayColor);

    // We default to lambert if no conversion was requested:
    const TfToken& preferredMaterial
        = jobArguments.preferredMaterial != UsdMayaPreferredMaterialTokens->none
        ? jobArguments.preferredMaterial
        : UsdMayaPreferredMaterialTokens->lambert;
    std::string shaderName(preferredMaterial.GetText());
    SdfPath     shaderParentPath = SdfPath::AbsoluteRootPath();
    if (shadeMaterial) {
        const UsdPrim& shadeMaterialPrim = shadeMaterial.GetPrim();
        shaderName = TfStringPrintf(
            "%s_%s", shadeMaterialPrim.GetName().GetText(), preferredMaterial.GetText());
        shaderParentPath = shadeMaterialPrim.GetPath();
    }

    // Construct the selected shader.
    MObject shadingObj;
    UsdMayaTranslatorUtil::CreateShaderNode(
        MString(shaderName.c_str()),
        preferredMaterial.GetText(),
        UsdMayaShadingNodeType::Shader,
        &status,
        &shadingObj);
    if (status != MS::kSuccess) {
        TF_RUNTIME_ERROR(
            "Could not create node of type '%s' for prim '%s'.\n",
            preferredMaterial.GetText(),
            primSchema.GetPath().GetText());
        return MObject();
    }

    MPlug       outputPlug;
    std::string surfaceNodeName;
#if MAYA_API_VERSION >= 20200000
    if (preferredMaterial == UsdMayaPreferredMaterialTokens->standardSurface) {
        MFnStandardSurfaceShader surfaceFn;
        surfaceFn.setObject(shadingObj);
        surfaceNodeName = surfaceFn.name().asChar();
        surfaceFn.setBase(1.0f);
        surfaceFn.setBaseColor(MColor(displayColor[0], displayColor[1], displayColor[2]));

        surfaceFn.setTransmission(linearTransparency[0]);

        const SdfPath surfacePath
            = shaderParentPath.AppendChild(TfToken(surfaceFn.name().asChar()));
        context->AddCreatedObject(surfacePath, shadingObj);

        // Find the outColor plug so we can connect it as the surface shader of the
        // shading engine.
        outputPlug = surfaceFn.findPlug("outColor", &status);
        CHECK_MSTATUS_AND_RETURN(status, MObject());
    } else
#endif
        if (preferredMaterial == UsdMayaPreferredMaterialTokens->usdPreviewSurface) {
        MFnDependencyNode depNodeFn;
        depNodeFn.setObject(shadingObj);
        surfaceNodeName = depNodeFn.name().asChar();

        MPlug diffusePlug = depNodeFn.findPlug(_tokens->diffuseColor.GetText());
        UsdMayaReadUtil::SetMayaAttr(
            diffusePlug, VtValue(displayColor), /*unlinearizeColors*/ false);

        MPlug opacityPlug = depNodeFn.findPlug(_tokens->opacity.GetText());
        UsdMayaReadUtil::SetMayaAttr(
            opacityPlug, VtValue(1.0f - linearTransparency[0]), /*unlinearizeColors*/ false);

        outputPlug = depNodeFn.findPlug("outColor", &status);
        CHECK_MSTATUS_AND_RETURN(status, MObject());
    } else {
        MFnLambertShader lambertFn;
        lambertFn.setObject(shadingObj);
        surfaceNodeName = lambertFn.name().asChar();
        lambertFn.setColor(MColor(displayColor[0], displayColor[1], displayColor[2]));
        lambertFn.setTransparency(
            MColor(linearTransparency[0], linearTransparency[1], linearTransparency[2]));

        // We explicitly set diffuse coefficient to 1.0 here since new lamberts
        // default to 0.8. This is to make sure the color value matches visually
        // when roundtripping since we bake the diffuseCoeff into the diffuse color
        // at export.
        lambertFn.setDiffuseCoeff(1.0);

        const SdfPath lambertPath
            = shaderParentPath.AppendChild(TfToken(lambertFn.name().asChar()));
        context->AddCreatedObject(lambertPath, shadingObj);

        outputPlug = lambertFn.findPlug("outColor", &status);
        CHECK_MSTATUS_AND_RETURN(status, MObject());
    }
    // Create the shading engine.
    MObject shadingEngine = context->CreateShadingEngine(surfaceNodeName);
    if (shadingEngine.isNull()) {
        return MObject();
    }
    MFnSet fnSet(shadingEngine, &status);
    if (status != MS::kSuccess) {
        return MObject();
    }

    const TfToken surfaceShaderPlugName = context->GetSurfaceShaderPlugName();
    if (!surfaceShaderPlugName.IsEmpty()) {
        MPlug seSurfaceShaderPlg = fnSet.findPlug(surfaceShaderPlugName.GetText(), &status);
        CHECK_MSTATUS_AND_RETURN(status, MObject());
        UsdMayaUtil::Connect(
            outputPlug,
            seSurfaceShaderPlg,
            /* clearDstPlug = */ true);
    }

    return shadingEngine;
}

PXR_NAMESPACE_CLOSE_SCOPE
