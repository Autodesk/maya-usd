//
// Copyright 2018 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http:#www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "usdPreviewSurface.h"

#include <maya/MFnAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnNumericAttribute.h>

PXR_NAMESPACE_OPEN_SCOPE

const MString MtohUsdPreviewSurface::classification("shader/surface");
const MString MtohUsdPreviewSurface::name("UsdPreviewSurface");
const MTypeId MtohUsdPreviewSurface::typeId(
    0x00116EFB); // something from our luma IDS

MStatus MtohUsdPreviewSurface::Initialize() {
    MFnEnumAttribute eAttr;
    MFnNumericAttribute nAttr;

    auto outColor = nAttr.createColor("outColor", "out");
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    nAttr.setReadable(true);
    nAttr.setWritable(false);
    addAttribute(outColor);

    auto outAlpha = nAttr.create("outAlpha", "outa", MFnNumericData::kFloat);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    nAttr.setReadable(true);
    nAttr.setWritable(false);
    addAttribute(outAlpha);

    // This is good for some old style compute implementation.
    auto normalCamera = nAttr.createPoint("normalCamera", "n");
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    nAttr.setReadable(true);
    nAttr.setWritable(true);
    nAttr.setDefault(1.0f, 1.0f, 1.0f);
    nAttr.setHidden(true);
    addAttribute(normalCamera);

    // Just some approximate defaults.
    auto roughness =
        nAttr.create("roughness", "roughness", MFnNumericData::kFloat);
    nAttr.setDefault(0.01f);
    nAttr.setMin(0.0f);
    nAttr.setMax(1.0f);
    addAttribute(roughness);

    auto clearcoat =
        nAttr.create("clearcoat", "clearcoat", MFnNumericData::kFloat);
    nAttr.setDefault(0.0f);
    nAttr.setMin(0.0f);
    nAttr.setMax(1.0f);
    addAttribute(clearcoat);

    auto clearcoatRoughness = nAttr.create(
        "clearcoatRoughness", "clearcoatRoughness", MFnNumericData::kFloat);
    nAttr.setDefault(0.01f);
    nAttr.setMin(0.0f);
    nAttr.setMax(1.0f);
    addAttribute(clearcoatRoughness);

    auto emissiveColor = nAttr.createColor("emissiveColor", "emissiveColor");
    nAttr.setDefault(0.0f, 0.0f, 0.0f);
    addAttribute(emissiveColor);

    auto specularColor = nAttr.createColor("specularColor", "specularColor");
    nAttr.setDefault(1.0f, 1.0f, 1.0f);
    addAttribute(specularColor);

    auto metallic =
        nAttr.create("metallic", "metallic", MFnNumericData::kFloat);
    nAttr.setDefault(1.0f);
    nAttr.setMin(0.0f);
    nAttr.setMax(1.0f);
    addAttribute(metallic);

    auto useSpecularWorkflow =
        eAttr.create("useSpecularWorkflow", "useSpecularWorkflow");
    eAttr.addField("metallic", 0);
    eAttr.addField("specular", 1);
    eAttr.setDefault(0);
    addAttribute(useSpecularWorkflow);

    auto occlusion =
        nAttr.create("occlusion", "occlusion", MFnNumericData::kFloat);
    nAttr.setDefault(1.0f);
    nAttr.setMin(0.0f);
    nAttr.setMax(1.0f);
    addAttribute(occlusion);

    auto ior = nAttr.create("ior", "ior", MFnNumericData::kFloat);
    nAttr.setDefault(1.5f);
    nAttr.setMin(0.0f);
    nAttr.setSoftMax(5.0f);
    addAttribute(ior);

    auto normal = nAttr.createPoint("normal", "normal");
    nAttr.setDefault(0.0f, 0.0f, 1.0f);
    addAttribute(normal);

    auto opacity = nAttr.create("opacity", "opacity", MFnNumericData::kFloat);
    nAttr.setDefault(1.0f);
    nAttr.setMin(0.0f);
    nAttr.setMax(1.0f);
    addAttribute(opacity);

    auto diffuseColor = nAttr.createColor("diffuseColor", "diffuseColor");
    nAttr.setDefault(0.18f, 0.18f, 0.18f);
    addAttribute(diffuseColor);

    auto displacement =
        nAttr.create("displacement", "displacement", MFnNumericData::kFloat);
    nAttr.setDefault(0.0f);
    nAttr.setSoftMin(0.0f);
    nAttr.setSoftMax(1.0f);
    addAttribute(displacement);

    attributeAffects(normalCamera, outColor);
    attributeAffects(roughness, outColor);
    attributeAffects(clearcoat, outColor);
    attributeAffects(clearcoatRoughness, outColor);
    attributeAffects(emissiveColor, outColor);
    attributeAffects(specularColor, outColor);
    attributeAffects(metallic, outColor);
    attributeAffects(useSpecularWorkflow, outColor);
    attributeAffects(occlusion, outColor);
    attributeAffects(ior, outColor);
    attributeAffects(normal, outColor);
    attributeAffects(opacity, outColor);
    attributeAffects(diffuseColor, outColor);
    attributeAffects(displacement, outColor);

    attributeAffects(normalCamera, outAlpha);
    attributeAffects(roughness, outAlpha);
    attributeAffects(clearcoat, outAlpha);
    attributeAffects(clearcoatRoughness, outAlpha);
    attributeAffects(emissiveColor, outAlpha);
    attributeAffects(specularColor, outAlpha);
    attributeAffects(metallic, outAlpha);
    attributeAffects(useSpecularWorkflow, outAlpha);
    attributeAffects(occlusion, outAlpha);
    attributeAffects(ior, outAlpha);
    attributeAffects(normal, outAlpha);
    attributeAffects(opacity, outAlpha);
    attributeAffects(diffuseColor, outAlpha);
    attributeAffects(displacement, outAlpha);

    return MS::kSuccess;
}

PXR_NAMESPACE_CLOSE_SCOPE
