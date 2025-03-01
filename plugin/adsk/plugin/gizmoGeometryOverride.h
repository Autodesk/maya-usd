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
#ifndef ASDK_PLUGIN_GIZMOGEOMETRYOVERRIDE_H
#define ASDK_PLUGIN_GIZMOGEOMETRYOVERRIDE_H

#include "base/api.h"

#include <mayaUsd/base/api.h>

#include <maya/MDagPath.h>
#include <maya/MDrawRegistry.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MGlobal.h>
#include <maya/MNodeMessage.h>
#include <maya/MPxGeometryOverride.h>
#include <maya/MShaderManager.h>
#include <maya/MStatus.h>
#include <maya/MTypeId.h>
#include <maya/MUintArray.h>
#include <maya/MUserData.h>

#include <iostream>
#include <vector>

namespace MAYAUSD_NS_DEF {

//! The user data that will be populated and consumed by the gizmo geometry override
class MAYAUSD_PLUGIN_PUBLIC GizmoData : public MUserData
{
public:
    enum ShapeType
    {
        Capsule,
        Circle,
        Cone,
        Cylinder,
        Distant,
        Dome,
        Point,
        Quad,
        Sphere
    };
    ShapeType fShapeType;
    float     fWidth;
    float     fHeight;
    float     fRadius;
    float     fPenumbraAngle;
    float     fConeAngle;
    float     fDropOff;
    float     fLightAngle;
};

class MAYAUSD_PLUGIN_PUBLIC GizmoGeometryOverride : public MHWRender::MPxGeometryOverride
{
public:
    GizmoGeometryOverride(const MObject& obj);
    virtual ~GizmoGeometryOverride();

    static MHWRender::MPxGeometryOverride* Creator(const MObject& obj)
    {
        return new GizmoGeometryOverride(obj);
    }
    virtual MHWRender::DrawAPI supportedDrawAPIs() const override { return (MHWRender::kAllDevices); }

    virtual void updateDG() override { }
    virtual bool isIndexingDirty(const MHWRender::MRenderItem& item) override { return true; }
    virtual bool isStreamDirty(const MHWRender::MVertexBufferDescriptor& desc) override { return true; }
    virtual void updateRenderItems(const MDagPath& path, MHWRender::MRenderItemList& list) override;
    virtual void populateGeometry(
        const MHWRender::MGeometryRequirements& requirements,
        const MHWRender::MRenderItemList&       renderItems,
        MHWRender::MGeometry&                   data) override;
    virtual void cleanUp() override { }

    static const MString dbClassification;

private:
    MHWRender::MShaderInstance* m_wireframeShader;
    MHWRender::MShaderInstance* m_activeWireframeShader;

    // Cached data
    MFloatVectorArray m_wirePositions;
    MUintArray        m_wireIndexing;
    bool              m_geometryDirty;
    float             m_scale[3];
    float             m_offset[3];
    float             m_wireframeColor[4];

    GizmoData::ShapeType fShapeType;
    float                fWidth;
    float                fHeight;
    float                fRadius;
    float                fPenumbraAngle;
    float                fConeAngle;
    float                fDropOff;
    float                fLightAngle;

    // Render item names
    static MString s_wireframeItemName;
    static MString s_activeWireframeItemName;

    // Associated object
    MObject m_Node;
};

} // namespace MAYAUSD_NS_DEF

#endif // ASDK_PLUGIN_GIZMOGEOMETRYOVERRIDE_H