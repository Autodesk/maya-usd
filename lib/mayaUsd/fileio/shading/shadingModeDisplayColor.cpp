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
#include <mayaUsd/fileio/shading/shadingModeExporter.h>
#include <mayaUsd/fileio/shading/shadingModeExporterContext.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/translators/translatorMaterial.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>
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
#include <maya/MFnReflectShader.h>
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

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (displayColor)
    (displayOpacity)
    (diffuseColor)
    (transmissionColor)
    (transparency)

    ((DefaultShaderId, "PxrDiffuse"))
    ((DefaultShaderOutputName, "out"))
);


namespace {
class DisplayColorShadingModeExporter : public UsdMayaShadingModeExporter
{
public:
    DisplayColorShadingModeExporter() {}
private:
    void Export(const UsdMayaShadingModeExportContext& context,
                UsdShadeMaterial * const mat,
                SdfPathSet * const boundPrimPaths) override
    {
        MStatus status;
        const MFnDependencyNode ssDepNode(context.GetSurfaceShader(), &status);
        if (status != MS::kSuccess) {
            return;
        }

        const UsdMayaShadingModeExportContext::AssignmentVector& assignments =
            context.GetAssignments();
        if (assignments.empty()) {
            return;
        }

        GfVec3f color;
        GfVec3f transparency;

        // We might use the Maya shading node's transparency to author a scalar
        // displayOpacity value on the UsdGeomGprim, as well as an input on the
        // shader prim. How we compute that value will depend on which Maya
        // shading node we're working with.
        float transparencyAvg;

        const MFnLambertShader lambertFn(ssDepNode.object(), &status);
        if (status == MS::kSuccess) {
            const MColor mayaColor = lambertFn.color();
            const MColor mayaTransparency = lambertFn.transparency();
            const float diffuseCoeff = lambertFn.diffuseCoeff();
            color = UsdMayaColorSpace::ConvertMayaToLinear(
                diffuseCoeff*GfVec3f(mayaColor[0], mayaColor[1], mayaColor[2]));
            transparency = UsdMayaColorSpace::ConvertMayaToLinear(
                GfVec3f(mayaTransparency[0], mayaTransparency[1], mayaTransparency[2]));
            // Compute the average transparency using the un-linearized Maya
            // value.
            transparencyAvg = (mayaTransparency[0] +
                               mayaTransparency[1] +
                               mayaTransparency[2]) / 3.0f;
        } else {
#if MAYA_API_VERSION >= 20200000
            const MFnStandardSurfaceShader surfaceFn(ssDepNode.object(), &status);
            if (status != MS::kSuccess) {
                return;
            }
            const MColor mayaColor = surfaceFn.baseColor();
            const float base = surfaceFn.base();
            color = UsdMayaColorSpace::ConvertMayaToLinear(
                base*GfVec3f(mayaColor[0], mayaColor[1], mayaColor[2]));
            const float mayaTransparency = surfaceFn.transmission();
            transparency = GfVec3f(mayaTransparency, mayaTransparency, mayaTransparency);
            // We can directly use the scalar transmission value as the
            // "average".
            transparencyAvg = mayaTransparency;
#else
            return;
#endif
        }

        VtVec3fArray displayColorAry;
        displayColorAry.push_back(color);

        VtFloatArray displayOpacityAry;
        if (transparencyAvg > 0.0f) {
            displayOpacityAry.push_back(1.0f - transparencyAvg);
        }

        const UsdStageRefPtr& stage = context.GetUsdStage();

        TF_FOR_ALL(iter, assignments) {
            const SdfPath& boundPrimPath = iter->first;
            const VtIntArray& faceIndices = iter->second;
            if (!faceIndices.empty()) {
                continue;
            }

            UsdPrim boundPrim = stage->GetPrimAtPath(boundPrimPath);
            if (!boundPrim) {
                TF_CODING_ERROR(
                        "Couldn't find bound prim <%s>",
                        boundPrimPath.GetText());
                continue;
            }

            if (boundPrim.IsInstance() || boundPrim.IsInstanceProxy()) {
                TF_WARN("Not authoring displayColor or displayOpacity for <%s> "
                        "because it is instanced", boundPrimPath.GetText());
                continue;
            }

            UsdGeomGprim primSchema(boundPrim);
            // Set color if not already authored
            if (!primSchema.GetDisplayColorAttr().HasAuthoredValue()) {
                // not animatable
                primSchema.CreateDisplayColorPrimvar().Set(displayColorAry);
            }
            if (transparencyAvg > 0.0f &&
                    !primSchema.GetDisplayOpacityAttr().HasAuthoredValue()) {
                // not animatable
                primSchema.CreateDisplayOpacityPrimvar().Set(displayOpacityAry);
            }
        }

        UsdPrim materialPrim = context.MakeStandardMaterialPrim(assignments,
                                                                std::string(),
                                                                boundPrimPaths);
        UsdShadeMaterial material(materialPrim);
        if (material) {
            *mat = material;

            // Create a Diffuse RIS shader for the Material.
            // Although Maya can't yet make use of it, downstream apps
            // can make use of Material interface inputs, so create one to
            // drive the shader's color.
            //
            // NOTE!  We do not set any values directly on the shaders;
            // instead we set the values only on the material's interface,
            // emphasizing that the interface is a value provider for
            // its shading networks.
            UsdShadeInput dispColorIA =
                material.CreateInput(_tokens->displayColor,
                                     SdfValueTypeNames->Color3f);
            dispColorIA.Set(VtValue(color));

            const std::string shaderName =
                TfStringPrintf("%s_lambert", materialPrim.GetName().GetText());
            const TfToken shaderPrimName(shaderName);
            UsdShadeShader shaderSchema =
                UsdShadeShader::Define(
                    stage,
                    materialPrim.GetPath().AppendChild(shaderPrimName));
            shaderSchema.CreateIdAttr(VtValue(_tokens->DefaultShaderId));
            UsdShadeInput diffuse =
                shaderSchema.CreateInput(_tokens->diffuseColor,
                                         SdfValueTypeNames->Color3f);

            diffuse.ConnectToSource(dispColorIA);

            // Make an interface input for transparency, which we will hook up
            // to the shader, and a displayOpacity, for any shader that might
            // want to consume it.  Only author a *value* if we got a
            // non-zero transparency
            UsdShadeInput transparencyIA =
                material.CreateInput(_tokens->transparency,
                                     SdfValueTypeNames->Color3f);
            UsdShadeInput dispOpacityIA =
                material.CreateInput(_tokens->displayOpacity,
                                     SdfValueTypeNames->Float);

            // PxrDiffuse's transmissionColor may not produce similar
            // results to MfnLambertShader's transparency, but it's in
            // the general ballpark...
            UsdShadeInput transmission =
                shaderSchema.CreateInput(_tokens->transmissionColor,
                                         SdfValueTypeNames->Color3f);
            transmission.ConnectToSource(transparencyIA);

            if (transparencyAvg > 0.0f) {
                transparencyIA.Set(VtValue(transparency));
                dispOpacityIA.Set(VtValue(1.0f - transparencyAvg));
            }

            UsdShadeOutput shaderDefaultOutput =
                shaderSchema.CreateOutput(_tokens->DefaultShaderOutputName,
                                          SdfValueTypeNames->Token);
            if (!shaderDefaultOutput) {
                return;
            }

            UsdRiMaterialAPI riMaterialAPI(materialPrim);
            riMaterialAPI.SetSurfaceSource(
                    shaderDefaultOutput.GetAttr().GetPath());
        }
    }
};
}

TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaShadingModeExportContext, displayColor)
{
    UsdMayaShadingModeRegistry::GetInstance().RegisterExporter(
        "displayColor",
        "Display Colors",
        "Exports the diffuse color of the bound shader as a displayColor primvar on the USD mesh.",
        []() -> UsdMayaShadingModeExporterPtr {
            return UsdMayaShadingModeExporterPtr(
                static_cast<UsdMayaShadingModeExporter*>(
                    new DisplayColorShadingModeExporter()));
        }
    );
}

DEFINE_SHADING_MODE_IMPORTER_WITH_JOB_ARGUMENTS(displayColor, context, jobArguments)
{
    const UsdShadeMaterial& shadeMaterial = context->GetShadeMaterial();
    const UsdGeomGprim& primSchema = context->GetBoundPrim();

    MStatus status;

    // Note that we always couple the source of the displayColor with the
    // source of the displayOpacity.  It would not make sense to get the
    // displayColor from a bound Material, while getting the displayOpacity from
    // the gprim itself, for example, even if the Material did not have
    // displayOpacity authored. When the Material or gprim does not have
    // displayOpacity authored, we fall back to full opacity.
    bool gotDisplayColorAndOpacity = false;

    // Get Display Color from USD (linear) and convert to Display
    GfVec3f linearDisplayColor(.5,.5,.5);
    GfVec3f linearTransparency(0, 0, 0);

    UsdShadeInput shadeInput = shadeMaterial ?
        shadeMaterial.GetInput(_tokens->displayColor) :
        UsdShadeInput();

    if (!shadeInput || !shadeInput.Get(&linearDisplayColor)) {

        VtVec3fArray gprimDisplayColor(1);
        if (primSchema &&
                primSchema.GetDisplayColorPrimvar().ComputeFlattened(&gprimDisplayColor)) {
            linearDisplayColor = gprimDisplayColor[0];
            VtFloatArray gprimDisplayOpacity(1);
            if (primSchema.GetDisplayOpacityPrimvar().GetAttr().HasAuthoredValue() &&
                    primSchema.GetDisplayOpacityPrimvar().ComputeFlattened(&gprimDisplayOpacity)) {
                const float trans = 1.0 - gprimDisplayOpacity[0];
                linearTransparency = GfVec3f(trans, trans, trans);
            }
            gotDisplayColorAndOpacity = true;
        } else {
            TF_WARN("Unable to retrieve displayColor on Material: %s "
                    "or Gprim: %s",
                    shadeMaterial ?
                        shadeMaterial.GetPrim().GetPath().GetText() :
                        "<NONE>",
                    primSchema ?
                        primSchema.GetPrim().GetPath().GetText() :
                        "<NONE>");
        }
    } else {
        shadeMaterial
            .GetInput(_tokens->transparency)
            .GetAttr()
            .Get(&linearTransparency);
        gotDisplayColorAndOpacity = true;
    }

    if (!gotDisplayColorAndOpacity) {
        return MObject();
    }

    const GfVec3f displayColor =
        UsdMayaColorSpace::ConvertLinearToMaya(linearDisplayColor);
    const GfVec3f transparencyColor =
        UsdMayaColorSpace::ConvertLinearToMaya(linearTransparency);

    // We default to lambert if no conversion was requested:
    const TfToken& shadingConversion
        = jobArguments.shadingConversion != UsdMayaShadingConversionTokens->none
        ? jobArguments.shadingConversion
        : UsdMayaShadingConversionTokens->lambert;
    std::string shaderName(shadingConversion.GetText());
    SdfPath shaderParentPath = SdfPath::AbsoluteRootPath();
    if (shadeMaterial) {
        const UsdPrim& shadeMaterialPrim = shadeMaterial.GetPrim();
        shaderName =
            TfStringPrintf("%s_%s",
                shadeMaterialPrim.GetName().GetText(),
                shadingConversion.GetText());
        shaderParentPath = shadeMaterialPrim.GetPath();
    }

    // Construct the selected shader.
    MObject           shadingObj;
    UsdMayaTranslatorUtil::CreateShaderNode(
              MString(shaderName.c_str()),
              shadingConversion.GetText(),
              UsdMayaShadingNodeType::Shader,
              &status,
              &shadingObj);
    if (status != MS::kSuccess) {
        TF_RUNTIME_ERROR(
            "Could not create node of type '%s' for prim '%s'.\n",
            shadingConversion.GetText(),
            primSchema.GetPath().GetText());
        return MObject();
    }

    MPlug outputPlug;
#if MAYA_API_VERSION >= 20200000
    if (shadingConversion != UsdMayaShadingConversionTokens->standardSurface) {
#endif
        MFnLambertShader lambertFn;
        lambertFn.setObject(shadingObj);
        lambertFn.setColor(
            MColor(displayColor[0], displayColor[1], displayColor[2]));
        lambertFn.setTransparency(
            MColor(transparencyColor[0], transparencyColor[1], transparencyColor[2]));
        
        // We explicitly set diffuse coefficient to 1.0 here since new lamberts
        // default to 0.8. This is to make sure the color value matches visually
        // when roundtripping since we bake the diffuseCoeff into the diffuse color
        // at export.
        lambertFn.setDiffuseCoeff(1.0);

        const SdfPath lambertPath =
            shaderParentPath.AppendChild(TfToken(lambertFn.name().asChar()));
        context->AddCreatedObject(lambertPath, shadingObj);

        outputPlug = lambertFn.findPlug("outColor", &status);
        CHECK_MSTATUS_AND_RETURN(status, MObject());
#if MAYA_API_VERSION >= 20200000
    } else {
        MFnStandardSurfaceShader surfaceFn;
        surfaceFn.setObject(shadingObj);
        surfaceFn.setBase(1.0f);
        surfaceFn.setBaseColor(
            MColor(displayColor[0], displayColor[1], displayColor[2]));

        float transmission
            = (transparencyColor[0] + transparencyColor[1] + transparencyColor[2]) / 3.0f;
        surfaceFn.setTransmission(transmission);

        const SdfPath surfacePath =
            shaderParentPath.AppendChild(TfToken(surfaceFn.name().asChar()));
        context->AddCreatedObject(surfacePath, shadingObj);

        // Find the outColor plug so we can connect it as the surface shader of the
        // shading engine.
        outputPlug = surfaceFn.findPlug("outColor", &status);
        CHECK_MSTATUS_AND_RETURN(status, MObject());
    }
#endif

    // Create the shading engine.
    MObject shadingEngine = context->CreateShadingEngine();
    if (shadingEngine.isNull()) {
        return MObject();
    }
    MFnSet fnSet(shadingEngine, &status);
    if (status != MS::kSuccess) {
        return MObject();
    }

    const TfToken surfaceShaderPlugName = context->GetSurfaceShaderPlugName();
    if (!surfaceShaderPlugName.IsEmpty()) {
        MPlug seSurfaceShaderPlg =
            fnSet.findPlug(surfaceShaderPlugName.GetText(), &status);
        CHECK_MSTATUS_AND_RETURN(status, MObject());
        UsdMayaUtil::Connect(outputPlug,
                                seSurfaceShaderPlg,
                                /* clearDstPlug = */ true);
    }

    return shadingEngine;
}


PXR_NAMESPACE_CLOSE_SCOPE
