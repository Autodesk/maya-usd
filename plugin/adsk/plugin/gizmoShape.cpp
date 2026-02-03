//
// Copyright 2025 Autodesk
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
#include "gizmoShape.h"

#include <maya/MFnData.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnLightDataAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MShaderManager.h>

namespace MAYAUSD_NS_DEF {

const MString GizmoShape::typeNamePrefix("ufeLight");
const MTypeId GizmoShape::idDefault(0x5800009C);
const MTypeId GizmoShape::idDistant(0x5800009D);
const MTypeId GizmoShape::idRect(0x5800009E);
const MTypeId GizmoShape::idDomeLight(0x5800009F);
// Skipping 0x580000A0 and 0x580000A1 since they are currently used by internal Maya
const MTypeId GizmoShape::idSphere(0x580000A2);
const MTypeId GizmoShape::idDisk(0x580000A3);
const MTypeId GizmoShape::idCone(0x580000A4);
const MTypeId GizmoShape::idCylinder(0x580000A5);

const MString
    GizmoShape::dbClassificationGeometryOverride("drawdb/geometry/mayaUsdGizmoGeometryOverride");
// Note: The first part of the classification is the to let Maya know that we want to use Maya's own
// internal light shading. The second part matches the override we registered for the gizmo.
const MString GizmoShape::dbClassificationDefault(
    MString("light:drawdb/light/pointLight") + ":" + dbClassificationGeometryOverride);
const MString GizmoShape::dbClassificationDistant(
    MString("light:drawdb/light/directionalLight") + ":" + dbClassificationGeometryOverride);
const MString GizmoShape::dbClassificationRect(
    MString("light:drawdb/light/areaLight") + ":" + dbClassificationGeometryOverride);
const MString GizmoShape::dbClassificationShapingAPICone(
    MString("light:drawdb/light/spotLight") + ":" + dbClassificationGeometryOverride);

MObject GizmoShape::ufePath;
MObject GizmoShape::shapeType;
MObject GizmoShape::width;
MObject GizmoShape::height;
MObject GizmoShape::radius;
MObject GizmoShape::penumbraAngle;
MObject GizmoShape::coneAngle;
MObject GizmoShape::dropOff;
MObject GizmoShape::lightAngle;

// Internal Maya Light Input attributes
MObject GizmoShape::aColor;
MObject GizmoShape::aIntensity;
MObject GizmoShape::aExposure;
MObject GizmoShape::aEmitDiffuse;
MObject GizmoShape::aEmitSpecular;
MObject GizmoShape::aDecayRate;
MObject GizmoShape::aLocatorScale;

// Output attributes
MObject GizmoShape::aOutColor;

#define MAKE_INPUT(attr)                   \
    CHECK_MSTATUS(attr.setKeyable(true));  \
    CHECK_MSTATUS(attr.setStorable(true)); \
    CHECK_MSTATUS(attr.setReadable(true)); \
    CHECK_MSTATUS(attr.setWritable(true)); \
    CHECK_MSTATUS(attr.setAffectsAppearance(true));

#define MAKE_OUTPUT(attr)                   \
    CHECK_MSTATUS(attr.setKeyable(false));  \
    CHECK_MSTATUS(attr.setStorable(false)); \
    CHECK_MSTATUS(attr.setReadable(true));  \
    CHECK_MSTATUS(attr.setWritable(false));

void GizmoShape::postConstructor()
{
    // This call allows the shape to have shading groups assigned
    setRenderable(true);

    MObject me = thisMObject();
    nodeDirtyId = MNodeMessage::addNodeDirtyPlugCallback(me, nodeDirtyEventCallback, this);
}

// Callback to trigger dirty for VP2 draw
void GizmoShape::nodeDirtyEventCallback(MObject& node, MPlug& plug, void* clientData)
{
    if (plug.attribute() == shapeType || plug.attribute() == width || plug.attribute() == height
        || plug.attribute() == radius || plug.attribute() == penumbraAngle
        || plug.attribute() == coneAngle || plug.attribute() == dropOff
        || plug.attribute() == lightAngle || plug.attribute() == ufePath) {
        MHWRender::MRenderer::setGeometryDrawDirty(node);
    }
}

MStatus GizmoShape::compute(const MPlug& plug, MDataBlock& data)
{

    if ((plug != aOutColor) && (plug.parent() != aOutColor)) {
        return MS::kUnknownParameter;
    }

    // Set outColor be color
    MFloatVector  resultColor = data.inputValue(aColor).asFloatVector();
    MDataHandle   outColorHandle = data.outputValue(aOutColor);
    MFloatVector& outColor = outColorHandle.asFloatVector();
    outColor = resultColor;
    outColorHandle.setClean();

    return MS::kSuccess;
}

MStatus GizmoShape::initialize()
{
    MStatus               status;
    MFnEnumAttribute      enAttr;
    MFnNumericAttribute   nAttr;
    MFnLightDataAttribute lAttr;
    MFnTypedAttribute     tAttr;

    // Internal Maya Light Attributes
    aColor = nAttr.createColor("color", "cl");
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS(nAttr.setDefault(0.5f, 0.5f, 0.5f));

    aEmitDiffuse = nAttr.create("emitDiffuse", "ed", MFnNumericData::kBoolean);
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS(nAttr.setDefault(true));

    aEmitSpecular = nAttr.create("emitSpecular", "sn", MFnNumericData::kBoolean);
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS(nAttr.setDefault(true));

    aIntensity = nAttr.create("intensity", "i", MFnNumericData::kFloat);
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS(nAttr.setDefault(1.0f));

    aExposure = nAttr.create("exposure", "exp", MFnNumericData::kFloat);
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS(nAttr.setDefault(0.0f));

    aLocatorScale = nAttr.create("locatorScale", "lls", MFnNumericData::kDouble);
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS(nAttr.setDefault(1.0));

    aDecayRate = nAttr.create("decayRate", "de", MFnNumericData::kShort);
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS(nAttr.setDefault(0));

    CHECK_MSTATUS(addAttribute(aColor));
    CHECK_MSTATUS(addAttribute(aIntensity));
    CHECK_MSTATUS(addAttribute(aExposure));
    CHECK_MSTATUS(addAttribute(aEmitDiffuse));
    CHECK_MSTATUS(addAttribute(aEmitSpecular));
    CHECK_MSTATUS(addAttribute(aLocatorScale));
    CHECK_MSTATUS(addAttribute(aDecayRate));

    // Note that "oc" conflicts with objectColor so we use something else.
    aOutColor = nAttr.createColor("outColor", "ocl");
    MAKE_OUTPUT(nAttr);
    CHECK_MSTATUS(addAttribute(aOutColor));

    CHECK_MSTATUS(attributeAffects(aColor, aOutColor));
    CHECK_MSTATUS(attributeAffects(aIntensity, aOutColor));
    CHECK_MSTATUS(attributeAffects(aExposure, aOutColor));
    CHECK_MSTATUS(attributeAffects(aEmitDiffuse, aOutColor));
    CHECK_MSTATUS(attributeAffects(aEmitSpecular, aOutColor));

    // The ufePath is currently exclusive to shapeType::Quad. This is necessary
    // for retrieving the width / height directly from a UsdLuxRectLight / UsdLuxPortalLight as
    // Ufe::Light::AreaInterface currently doesn't include width / height attributes.
    ufePath = tAttr.create("ufePath", "ufePth", MFnData::kString, MObject::kNullObj, &status);
    CHECK_MSTATUS(status);
    tAttr.setStorable(true);
    tAttr.setKeyable(true);
    addAttribute(ufePath);

    shapeType = enAttr.create("shapeType", "shapeType", 0, &status);
    CHECK_MSTATUS(status);
    enAttr.addField("Capsule", 0);
    enAttr.addField("Circle", 1);
    enAttr.addField("Cone", 2);
    enAttr.addField("Cylinder", 3);
    enAttr.addField("Distant", 4);
    enAttr.addField("Dome", 5);
    enAttr.addField("Point", 6);
    enAttr.addField("Quad", 7);
    enAttr.addField("Sphere", 8);
    enAttr.setStorable(true);
    enAttr.setKeyable(true);
    addAttribute(shapeType);

    width = nAttr.create("width", "wdth", MFnNumericData::kFloat, 1.0, &status);
    CHECK_MSTATUS(status);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);
    addAttribute(width);

    height = nAttr.create("height", "ht", MFnNumericData::kFloat, 1.0, &status);
    CHECK_MSTATUS(status);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);
    addAttribute(height);

    radius = nAttr.create("radius", "rds", MFnNumericData::kFloat, 1.0, &status);
    CHECK_MSTATUS(status);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);
    addAttribute(radius);

    penumbraAngle = nAttr.create("penumbra", "pnmb", MFnNumericData::kFloat, 1.0, &status);
    CHECK_MSTATUS(status);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);
    addAttribute(penumbraAngle);

    coneAngle = nAttr.create("coneAngle", "coneAngle", MFnNumericData::kFloat, 1.0, &status);
    CHECK_MSTATUS(status);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);
    addAttribute(coneAngle);

    dropOff = nAttr.create("dropOff", "dropOff", MFnNumericData::kFloat, 1.0, &status);
    CHECK_MSTATUS(status);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);
    addAttribute(dropOff);

    lightAngle = nAttr.create("lightAngle", "lightAngle", MFnNumericData::kFloat, 1.0, &status);
    CHECK_MSTATUS(status);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);
    addAttribute(lightAngle);

    return MStatus::kSuccess;
}

MSelectionMask GizmoShape::getShapeSelectionMask() const
{
    MSelectionMask::SelectionType selType = MSelectionMask::kSelectMeshes;
    return selType;
}

} // namespace MAYAUSD_NS_DEF