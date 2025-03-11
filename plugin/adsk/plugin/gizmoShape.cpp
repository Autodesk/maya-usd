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

const MString GizmoShape::typeName("mayaUsdGeometryGizmoShape");
const MTypeId GizmoShape::id(0x5800019C);
MObject       GizmoShape::ufePath;
MObject       GizmoShape::shapeType;
MObject       GizmoShape::width;
MObject       GizmoShape::height;
MObject       GizmoShape::radius;
MObject       GizmoShape::penumbraAngle;
MObject       GizmoShape::coneAngle;
MObject       GizmoShape::dropOff;
MObject       GizmoShape::lightAngle;

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

MStatus GizmoShape::initialize()
{
    MStatus               status;
    MFnEnumAttribute      enAttr;
    MFnNumericAttribute   nAttr;
    MFnLightDataAttribute lAttr;
    MFnTypedAttribute     tAttr;

    // The ufePath is currently exclusive to shapeType::Quad. This is necessary
    // for retrieving the width / height directly from a UsdLuxRectLight as
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