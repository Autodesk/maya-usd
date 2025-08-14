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

    MStatus compute(const MPlug& plug, MDataBlock& data) override;

    static void*   creator() { return new GizmoShape(); }
    static MStatus initialize();

    MSelectionMask getShapeSelectionMask() const override;

    //! The node name that can be used to create the GizmoShape
    static const MString typeNamePrefix;
    //! The default typeId that supports Maya's internal Point light shading
    static const MTypeId idDefault;
    //! The typeId that supports Maya's internal Directional light shading
    static const MTypeId idDistant;
    //! The typeId that supports Maya's internal light shading
    static const MTypeId idRect;
    static const MTypeId idDomeLight;
    static const MTypeId idSphere;
    static const MTypeId idDisk;
    static const MTypeId idCone;
    static const MTypeId idCylinder;
    //! The classification string for the gizmo geometry override. This is used to draw custom
    //! gizmos for all light types. This is appended to the light classification string.
    static const MString dbClassificationGeometryOverride;

    static const MString dbClassificationDefault;
    static const MString dbClassificationDistant;
    static const MString dbClassificationRect;
    static const MString dbClassificationShapingAPICone;

private:
    MCallbackId nodeDirtyId;

    // Attributes
    static MObject ufePath;
    static MObject shapeType;
    static MObject width;
    static MObject height;
    static MObject radius;
    static MObject penumbraAngle;
    static MObject coneAngle;
    static MObject dropOff;
    static MObject lightAngle;

    // Input attributes to mimic a Maya internal light
    static MObject aColor;
    static MObject aIntensity;
    static MObject aExposure;
    static MObject aEmitDiffuse;
    static MObject aEmitSpecular;
    static MObject aLocatorScale;
    static MObject aDecayRate;
#if UFE_LIGHTS2_SUPPORT
    static MObject aNormalize;
    static MObject aUseRayTraceShadows;
#endif

    // General output color attribute
    static MObject aOutColor;

}; // class GizmoShape

} // namespace MAYAUSD_NS_DEF

#endif // ASDK_PLUGIN_GIZMOSHAPE_H