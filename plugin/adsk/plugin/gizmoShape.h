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
#ifndef ASDK_PLUGIN_GIZMOSHAPE_H
#define ASDK_PLUGIN_GIZMOSHAPE_H

#include "base/api.h"

#include <mayaUsd/base/api.h>

#include <maya/M3dView.h>
#include <maya/MDrawRegistry.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnLightDataAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MNodeMessage.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MStatus.h>
#include <maya/MTypeId.h>

#include <iostream>
#include <vector>

namespace MAYAUSD_NS_DEF {

//! Gizmo Shape class - defines the non-UI part of a shape node
class MAYAUSD_PLUGIN_PUBLIC GizmoShape : public MPxSurfaceShape
{

public:
    GizmoShape() { }

    virtual ~GizmoShape() { }

    void postConstructor() override;

    static void nodeDirtyEventCallback(MObject& node, MPlug& plug, void* clientData);

    MStatus compute(const MPlug& plug, MDataBlock& data) override { return MS::kUnknownParameter; }

    static void*   creator() { return new GizmoShape(); }
    static MStatus initialize();

    MSelectionMask getShapeSelectionMask() const override;

    //! The node name that can be used to create the GizmoShape
    static const MString typeName;
    static const MTypeId id;

private:
    MCallbackId nodeDirtyId;

    // Attributes
    static MObject shapeType;
    static MObject width;
    static MObject height;
    static MObject radius;
    static MObject penumbraAngle;
    static MObject coneAngle;
    static MObject dropOff;
    static MObject lightAngle;

}; // class GizmoShape

} // namespace MAYAUSD_NS_DEF

#endif // ASDK_PLUGIN_GIZMOSHAPE_H