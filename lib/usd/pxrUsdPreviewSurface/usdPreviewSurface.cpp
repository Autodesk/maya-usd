//
// Copyright 2018 Pixar
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
#include "usdPreviewSurface.h"

#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/pxr.h>

#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFloatVector.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPxNode.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MVector.h>

#include <limits>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(
    PxrMayaUsdPreviewSurfaceTokens,
    PXRUSDPREVIEWSURFACE_USD_PREVIEW_SURFACE_TOKENS);

/* static */
void* PxrMayaUsdPreviewSurface::creator() { return new PxrMayaUsdPreviewSurface(); }

/* static */
MStatus PxrMayaUsdPreviewSurface::initialize()
{
    MStatus status;

    MObject clearcoatAttr;
    MObject clearcoatRoughnessAttr;
    MObject diffuseColorAttr;
    MObject displacementAttr;
    MObject emissiveColorAttr;
    MObject iorAttr;
    MObject metallicAttr;
    MObject normalAttr;
    MObject occlusionAttr;
    MObject opacityAttr;
    MObject roughnessAttr;
    MObject specularColorAttr;
    MObject useSpecularWorkflowAttr;
    MObject outColorAttr;
    MObject outTransparencyAttr;

    MFnNumericAttribute numericAttrFn;

    clearcoatAttr = numericAttrFn.create(
        PxrMayaUsdPreviewSurfaceTokens->ClearcoatAttrName.GetText(),
        "cc",
        MFnNumericData::kFloat,
        0.0,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMin(0.0);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMax(1.0);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(clearcoatAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    clearcoatRoughnessAttr = numericAttrFn.create(
        PxrMayaUsdPreviewSurfaceTokens->ClearcoatRoughnessAttrName.GetText(),
        "ccr",
        MFnNumericData::kFloat,
        0.01,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setMin(0.001);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMin(0.001);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMax(1.0);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(clearcoatRoughnessAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    diffuseColorAttr = numericAttrFn.createColor(
        PxrMayaUsdPreviewSurfaceTokens->DiffuseColorAttrName.GetText(), "dc", &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setDefault(0.18f, 0.18f, 0.18f);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(diffuseColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    displacementAttr = numericAttrFn.create(
        PxrMayaUsdPreviewSurfaceTokens->DisplacementAttrName.GetText(),
        "dsp",
        MFnNumericData::kFloat,
        0.0,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(displacementAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    emissiveColorAttr = numericAttrFn.createColor(
        PxrMayaUsdPreviewSurfaceTokens->EmissiveColorAttrName.GetText(), "ec", &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(emissiveColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    iorAttr = numericAttrFn.create(
        PxrMayaUsdPreviewSurfaceTokens->IorAttrName.GetText(),
        "ior",
        MFnNumericData::kFloat,
        1.5,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(iorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    metallicAttr = numericAttrFn.create(
        PxrMayaUsdPreviewSurfaceTokens->MetallicAttrName.GetText(),
        "mtl",
        MFnNumericData::kFloat,
        0.0,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMin(0.0);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMax(1.0);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(metallicAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    normalAttr = numericAttrFn.create(
        PxrMayaUsdPreviewSurfaceTokens->NormalAttrName.GetText(),
        "nrm",
        MFnNumericData::k3Float,
        0.0,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    const MVector upAxis = MGlobal::upAxis(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setDefault(upAxis[0], upAxis[1], upAxis[2]);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(normalAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    occlusionAttr = numericAttrFn.create(
        PxrMayaUsdPreviewSurfaceTokens->OcclusionAttrName.GetText(),
        "ocl",
        MFnNumericData::kFloat,
        1.0,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMin(0.0);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMax(1.0);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(occlusionAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    opacityAttr = numericAttrFn.create(
        PxrMayaUsdPreviewSurfaceTokens->OpacityAttrName.GetText(),
        "opc",
        MFnNumericData::kFloat,
        1.0,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMin(0.0);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMax(1.0);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(opacityAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    roughnessAttr = numericAttrFn.create(
        PxrMayaUsdPreviewSurfaceTokens->RoughnessAttrName.GetText(),
        "rgh",
        MFnNumericData::kFloat,
        0.5,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setMin(0.001);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMin(0.001);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMax(1.0);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(roughnessAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    specularColorAttr = numericAttrFn.createColor(
        PxrMayaUsdPreviewSurfaceTokens->SpecularColorAttrName.GetText(), "spc", &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(specularColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    useSpecularWorkflowAttr = numericAttrFn.create(
        PxrMayaUsdPreviewSurfaceTokens->UseSpecularWorkflowAttrName.GetText(),
        "usw",
        MFnNumericData::kBoolean,
        0.0,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(useSpecularWorkflowAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    outColorAttr = numericAttrFn.createColor(
        PxrMayaUsdPreviewSurfaceTokens->OutColorAttrName.GetText(), "oc", &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setWritable(false);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setStorable(false);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    outTransparencyAttr = numericAttrFn.createColor(
        PxrMayaUsdPreviewSurfaceTokens->OutTransparencyAttrName.GetText(), "ot", &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setWritable(false);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setStorable(false);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(outTransparencyAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject outTransparencyOnAttr = numericAttrFn.create(
        PxrMayaUsdPreviewSurfaceTokens->OutTransparencyOnAttrName.GetText(),
        "oto",
        MFnNumericData::kFloat,
        0.0,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setHidden(true); // It is an implementation detail that should be hidden.
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setWritable(false);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setStorable(false);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(outTransparencyOnAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Note that we make *all* attributes affect "outColor". During export, we
    // use Maya's MItDependencyGraph iterator to traverse connected plugs
    // upstream in the network beginning at the shading engine's shader plugs
    // (e.g. "surfaceShader"). The iterator will not traverse plugs that it
    // does not know affect connections downstream. For example, if this shader
    // has connections for both "diffuseColor" and "roughness", but we only
    // declared the attribute affects relationship for "diffuseColor", then
    // only "diffuseColor" would be visited and "roughness" would be skipped
    // during the traversal, since the plug upstream of the shading engine's
    // "surfaceShader" plug is this shader's "outColor" attribute, which Maya
    // knows is affected by "diffuseColor".
    status = attributeAffects(clearcoatAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(clearcoatRoughnessAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(diffuseColorAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(displacementAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(emissiveColorAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(iorAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(metallicAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(normalAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(occlusionAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(opacityAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(roughnessAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(specularColorAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(useSpecularWorkflowAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = attributeAffects(opacityAttr, outTransparencyAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = attributeAffects(opacityAttr, outTransparencyOnAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return status;
}

/* virtual */
void PxrMayaUsdPreviewSurface::postConstructor() { setMPSafe(true); }

/* virtual */
MStatus PxrMayaUsdPreviewSurface::compute(const MPlug& plug, MDataBlock& dataBlock)
{
    MStatus status = MS::kUnknownParameter;

    // XXX: For now, we simply propagate diffuseColor to outColor and
    // opacity to outTransparency.
    MFnDependencyNode depNodeFn(thisMObject());
    MObject           outColorAttr
        = depNodeFn.attribute(PxrMayaUsdPreviewSurfaceTokens->OutColorAttrName.GetText());
    MObject outTransparencyAttr
        = depNodeFn.attribute(PxrMayaUsdPreviewSurfaceTokens->OutTransparencyAttrName.GetText());
    MObject outTransparencyOnAttr
        = depNodeFn.attribute(PxrMayaUsdPreviewSurfaceTokens->OutTransparencyOnAttrName.GetText());
    if (plug == outColorAttr) {
        MObject diffuseColorAttr
            = depNodeFn.attribute(PxrMayaUsdPreviewSurfaceTokens->DiffuseColorAttrName.GetText());
        const MDataHandle diffuseColorData = dataBlock.inputValue(diffuseColorAttr, &status);
        CHECK_MSTATUS(status);
        const MFloatVector diffuseColor = diffuseColorData.asFloatVector();

        MDataHandle outColorHandle = dataBlock.outputValue(outColorAttr, &status);
        CHECK_MSTATUS(status);
        outColorHandle.asFloatVector() = diffuseColor;
        status = dataBlock.setClean(outColorAttr);
        CHECK_MSTATUS(status);
    } else if (plug == outTransparencyAttr) {
        MObject opacityAttr
            = depNodeFn.attribute(PxrMayaUsdPreviewSurfaceTokens->OpacityAttrName.GetText());
        const MDataHandle opacityData = dataBlock.inputValue(opacityAttr, &status);
        CHECK_MSTATUS(status);
        const float opacity = opacityData.asFloat();

        const float        transparency = 1.0f - opacity;
        const MFloatVector transparencyColor(transparency, transparency, transparency);
        MDataHandle outTransparencyHandle = dataBlock.outputValue(outTransparencyAttr, &status);
        CHECK_MSTATUS(status);
        outTransparencyHandle.asFloatVector() = transparencyColor;
        status = dataBlock.setClean(outTransparencyAttr);
        CHECK_MSTATUS(status);
    } else if (plug == outTransparencyOnAttr) {
        // The hidden "outTransparencyOn" attribute is a workaround for VP2 to execute transparency
        // test, see PxrMayaUsdPreviewSurfaceShadingNodeOverride::getCustomMappings() for more
        // details. We don't use the user-visible "outTransparency" attribute for transparency test
        // because its value depends on upstream nodes and thus error-prone when the "opacity" plug
        // is connected to certain textures. In that case, we should enable transparency.
        bool opacityConnected = false;

        const MObject opacityAttr
            = depNodeFn.attribute(PxrMayaUsdPreviewSurfaceTokens->OpacityAttrName.GetText());
        const MPlug opacityPlug(thisMObject(), opacityAttr);
        if (opacityPlug.isConnected()) {
            const MPlug sourcePlug = opacityPlug.source(&status);
            CHECK_MSTATUS(status);
            const MObject sourceNode = sourcePlug.node(&status);
            CHECK_MSTATUS(status);

            // Anim curve output will be evaluated to determine if transparency should be enabled.
            if (!sourceNode.hasFn(MFn::kAnimCurve)) {
                opacityConnected = true;
            }
        }

        float transparencyOn = false;
        if (opacityConnected) {
            transparencyOn = true;
        } else {
            const MDataHandle opacityData = dataBlock.inputValue(opacityAttr, &status);
            CHECK_MSTATUS(status);
            const float opacity = opacityData.asFloat();
            if (opacity < 1.0f - std::numeric_limits<float>::epsilon()) {
                transparencyOn = true;
            }
        }

        MDataHandle dataHandle = dataBlock.outputValue(outTransparencyOnAttr, &status);
        CHECK_MSTATUS(status);
        dataHandle.setFloat(transparencyOn ? 1.0f : 0.0f);
        status = dataBlock.setClean(outTransparencyOnAttr);
        CHECK_MSTATUS(status);
    }

    return status;
}

PxrMayaUsdPreviewSurface::PxrMayaUsdPreviewSurface()
    : MPxNode()
{
}

/* virtual */
PxrMayaUsdPreviewSurface::~PxrMayaUsdPreviewSurface() { }

PXR_NAMESPACE_CLOSE_SCOPE
