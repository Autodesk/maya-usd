#include "usdPreviewSurface.h"

#include <maya/MFnAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnNumericAttribute.h>

PXR_NAMESPACE_OPEN_SCOPE

const MString HdMayaUsdPreviewSurface::classification("shader/surface");
const MString HdMayaUsdPreviewSurface::name("UsdPreviewSurface");
const MTypeId HdMayaUsdPreviewSurface::typeId(0x00116EFB); // something from our luma IDS

MStatus
HdMayaUsdPreviewSurface::Initialize() {
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
    auto roughness = nAttr.create("roughness", "roughness", MFnNumericData::kFloat);
    nAttr.setDefault(0.0f);
    nAttr.setMin(0.0f);
    nAttr.setMax(1.0f);
    addAttribute(roughness);

    auto clearcoat = nAttr.create("clearcoat", "clearcoat", MFnNumericData::kFloat);
    nAttr.setDefault(0.0f);
    nAttr.setMin(0.0f);
    nAttr.setMax(1.0f);
    addAttribute(clearcoat);

    auto clearcoatRoughness = nAttr.create("clearcoatRoughness", "clearcoatRoughness", MFnNumericData::kFloat);
    nAttr.setDefault(0.0f);
    nAttr.setMin(0.0f);
    nAttr.setMax(1.0f);
    addAttribute(clearcoatRoughness);

    auto emissiveColor = nAttr.createColor("emissiveColor", "emissiveColor");
    nAttr.setDefault(0.0f, 0.0f, 0.0f);
    addAttribute(emissiveColor);

    auto specularColor = nAttr.createColor("specularColor", "specularColor");
    nAttr.setDefault(0.0f, 0.0f, 0.0f);
    addAttribute(specularColor);

    auto metallic = nAttr.create("metallic", "metallic", MFnNumericData::kFloat);
    nAttr.setDefault(0.0f);
    nAttr.setMin(0.0f);
    nAttr.setMax(1.0f);
    addAttribute(metallic);

    auto useSpecularWorkflow = eAttr.create("useSpecularWorkflow", "useSpecularWorkflow");
    eAttr.addField("metallic", 0);
    eAttr.addField("specular", 1);
    eAttr.setDefault(1);
    addAttribute(useSpecularWorkflow);

    auto occlusion = nAttr.create("occlusion", "occlusion", MFnNumericData::kFloat);
    nAttr.setDefault(1.0f);
    nAttr.setMin(0.0f);
    nAttr.setMax(1.0f);
    addAttribute(occlusion);

    auto ior = nAttr.create("ior", "ior", MFnNumericData::kFloat);
    nAttr.setDefault(1.0f);
    nAttr.setMin(0.0f);
    nAttr.setSoftMax(5.0f);
    addAttribute(ior);

    auto normal = nAttr.createPoint("normal", "normal");
    nAttr.setDefault(1.0f, 1.0f, 1.0f);
    addAttribute(normal);

    auto opacity = nAttr.create("opacity", "opacity", MFnNumericData::kFloat);
    nAttr.setDefault(1.0f);
    nAttr.setMin(0.0f);
    nAttr.setMax(1.0f);
    addAttribute(opacity);

    auto diffuseColor = nAttr.createColor("diffuseColor", "diffuseColor");
    nAttr.setDefault(1.0f, 1.0f, 1.0f);
    addAttribute(diffuseColor);

    auto displacement = nAttr.create("displacement", "displacement", MFnNumericData::kFloat);
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
