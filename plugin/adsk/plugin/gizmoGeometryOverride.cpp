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
#include "gizmoGeometryOverride.h"

#include "drawUtils.h"

#include <mayaUsd/ufe/Utils.h>

#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdLux/portalLight.h>
#include <pxr/usd/usdLux/rectLight.h>

#include <maya/M3dView.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MHWGeometry.h>
#include <maya/MHWGeometryUtilities.h>
#include <ufe/pathString.h>

namespace {
static MString colorParameterName_ = "solidColor";
}

namespace MAYAUSD_NS_DEF {

// Classification for drawing the gizmo
const MString
        GizmoGeometryOverride::dbClassification("drawdb/geometry/mayaUsdGizmoGeometryOverride");
MString GizmoGeometryOverride::s_wireframeItemName = "Gizmo";
MString GizmoGeometryOverride::s_activeWireframeItemName = "active_Gizmo";

GizmoGeometryOverride::GizmoGeometryOverride(const MObject& obj)
    : MHWRender::MPxGeometryOverride(obj)
    , m_geometryDirty(true)
    , fWidth(1.0f)
    , fHeight(1.0f)
    , fRadius(1.0f)
    , fPenumbraAngle(1.0f)
    , fConeAngle(1.0f)
    , fDropOff(1.0f)
    , fLightAngle(1.0f)
{
    m_scale[0] = m_scale[1] = m_scale[2] = 1.0f;
    m_offset[0] = m_offset[1] = m_offset[2] = 0.0f;
    m_wireframeColor[0] = m_wireframeColor[1] = m_wireframeColor[2] = m_wireframeColor[3];
    // Acquire resources
    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (renderer) {
        const MHWRender::MShaderManager* shaderMgr = renderer->getShaderManager();
        if (shaderMgr) {
            m_wireframeShader
                = shaderMgr->getStockShader(MHWRender::MShaderManager::k3dSolidShader);
            m_activeWireframeShader
                = shaderMgr->getStockShader(MHWRender::MShaderManager::k3dSolidShader);
        }
    }
}

GizmoGeometryOverride::~GizmoGeometryOverride()
{
    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (renderer) {
        // Release shaders
        const MHWRender::MShaderManager* shaderMgr = renderer->getShaderManager();
        if (shaderMgr) {
            if (m_wireframeShader) {
                shaderMgr->releaseShader(m_wireframeShader);
                m_wireframeShader = 0;
            }
            if (m_activeWireframeShader) {
                shaderMgr->releaseShader(m_activeWireframeShader);
                m_activeWireframeShader = 0;
            }
        }
    }
}

void GizmoGeometryOverride::updateRenderItems(
    const MDagPath&             path,
    MHWRender::MRenderItemList& list)
{
    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer)
        return;

    const MHWRender::MShaderManager* shaderMgr = renderer->getShaderManager();
    if (!shaderMgr)
        return;

    MStatus           status;
    MFnDependencyNode depNode(path.node(), &status);
    if (MStatus::kSuccess == status) {
        // Get the UFE path to retrieve the USD prim in case some parameters
        // are missing from the UFE scene item itself.
        MPlug ufePathPlug = depNode.findPlug("ufePath", true, &status);
        if (!ufePathPlug.isNull()) {
            MString val;
            status = ufePathPlug.getValue(val);
            if (MStatus::kSuccess == status) {
                m_geometryDirty = true;
                fUfePath = val;
            }
        }

        MPlug shapeTypePlug = depNode.findPlug("shapeType", true, &status);
        if (!shapeTypePlug.isNull()) {
            int val;
            status = shapeTypePlug.getValue(val);
            if (MStatus::kSuccess == status) {
                m_geometryDirty = m_geometryDirty || (((int)fShapeType) != val);
                fShapeType = (GizmoData::ShapeType)val;
            }
        }

        // By default, we use the width and height plugs to update the geometry.
        bool updateWidthHeightFromPlugs = true;

        // Special case for Ufe::Light::AreaInterface as it doesn't have width / height members
        // which are needed for UsdLuxRectLight and UsdLuxPortalLight
        if (fShapeType == GizmoData::ShapeType::Quad) {
            try {
                const Ufe::Path ufePath = Ufe::PathString::path(std::string(fUfePath.asChar()));
                auto            prim = MayaUsd::ufe::ufePathToPrim(ufePath);
                if (prim && prim.IsA<PXR_NS::UsdLuxRectLight>()) {
                    const PXR_NS::UsdLuxRectLight rectLight(prim);
                    const PXR_NS::UsdAttribute    widthAttribute = rectLight.GetWidthAttr();
                    const PXR_NS::UsdAttribute    heightAttribute = rectLight.GetHeightAttr();

                    widthAttribute.Get(&fWidth);
                    heightAttribute.Get(&fHeight);
                    // We've already set fWidth and fHeight, skip reading the width / height plugs.
                    updateWidthHeightFromPlugs = false;
                }
                // UsdLuxPortalLight:: GetWidthAttr and GetHeightAttr were only added after USD
                // v23.11
#if PXR_VERSION >= 2311
                else if (prim && prim.IsA<PXR_NS::UsdLuxPortalLight>()) {
                    const PXR_NS::UsdLuxPortalLight portalLight(prim);
                    const PXR_NS::UsdAttribute      widthAttribute = portalLight.GetWidthAttr();
                    const PXR_NS::UsdAttribute      heightAttribute = portalLight.GetHeightAttr();

                    widthAttribute.Get(&fWidth);
                    heightAttribute.Get(&fHeight);
                    // We've already set fWidth and fHeight, skip reading the width / height plugs.
                    updateWidthHeightFromPlugs = false;
                }
#endif
            } catch (const std::exception&) {
                // Do nothing
            }

            // No else clause here as we want to allow the normal work flow for all other cases of
            // ShapeType::Quad
        }

        if (updateWidthHeightFromPlugs) {
            MPlug widthPlug = depNode.findPlug("width", true, &status);
            if (!widthPlug.isNull()) {
                float val;
                status = widthPlug.getValue(val);
                if (MStatus::kSuccess == status) {
                    m_geometryDirty = m_geometryDirty || (fWidth != val);
                    fWidth = val;
                }
            }

            MPlug heightPlug = depNode.findPlug("height", true, &status);
            if (!heightPlug.isNull()) {
                float val;
                status = heightPlug.getValue(val);
                if (MStatus::kSuccess == status) {
                    m_geometryDirty = m_geometryDirty || (fHeight != val);
                    fHeight = val;
                }
            }
        }

        MPlug radiusPlug = depNode.findPlug("radius", true, &status);
        if (!radiusPlug.isNull()) {
            float val;
            status = radiusPlug.getValue(val);
            if (MStatus::kSuccess == status) {
                m_geometryDirty = m_geometryDirty || (fRadius != val);
                fRadius = val;
            }
        }

        MPlug penumbraPlug = depNode.findPlug("penumbra", true, &status);
        if (!penumbraPlug.isNull()) {
            float val;
            status = penumbraPlug.getValue(val);
            if (MStatus::kSuccess == status) {
                m_geometryDirty = m_geometryDirty || (fPenumbraAngle != val);
                fPenumbraAngle = val;
            }
        }

        MPlug coneAnglePlug = depNode.findPlug("coneAngle", true, &status);
        if (!coneAnglePlug.isNull()) {
            float val;
            status = coneAnglePlug.getValue(val);
            if (MStatus::kSuccess == status) {
                m_geometryDirty = m_geometryDirty || (fConeAngle != val);
                fConeAngle = val;
            }
        }

        MPlug dropOffPlug = depNode.findPlug("dropOff", true, &status);
        if (!dropOffPlug.isNull()) {
            float val;
            status = dropOffPlug.getValue(val);
            if (MStatus::kSuccess == status) {
                m_geometryDirty = m_geometryDirty || (fDropOff != val);
                fDropOff = val;
            }
        }

        MPlug lightAnglePlug = depNode.findPlug("lightAngle", true, &status);
        if (!lightAnglePlug.isNull()) {
            float val;
            status = lightAnglePlug.getValue(val);
            if (MStatus::kSuccess == status) {
                m_geometryDirty = m_geometryDirty || (fLightAngle != val);
                fLightAngle = val;
            }
        }
    }

    // Disable when not required
    bool                     needActiveItem = false;
    MHWRender::DisplayStatus displayStatus = MHWRender::MGeometryUtilities::displayStatus(path);
    switch (displayStatus) {
    case MHWRender::kLead:
    case MHWRender::kActive:
    case MHWRender::kHilite:
    case MHWRender::kActiveComponent: needActiveItem = true; break;
    default: break;
    };

    // Get color
    MColor color = MHWRender::MGeometryUtilities::wireframeColor(path);
    m_wireframeColor[0] = color.r;
    m_wireframeColor[1] = color.g;
    m_wireframeColor[2] = color.b;
    m_wireframeColor[3] = color.a;

    // 1. Add in a dormant wireframe render item
    MHWRender::MRenderItem* wireframeItem = 0;
    int                     index = list.indexOf(s_wireframeItemName);
    if (index < 0) {
        wireframeItem = MHWRender::MRenderItem::Create(
            s_wireframeItemName,
            MHWRender::MRenderItem::DecorationItem,
            MHWRender::MGeometry::kLines);

        wireframeItem->setDrawMode(MHWRender::MGeometry::kAll);
        wireframeItem->depthPriority(MHWRender::MRenderItem::sDormantWireDepthPriority);
        wireframeItem->setObjectTypeExclusionFlag(MHWRender::MFrameContext::kExcludeLights);
        wireframeItem->enable(true);
        wireframeItem->setCompatibleWithMayaInstancer(true);
        list.append(wireframeItem);
    } else {
        wireframeItem = list.itemAt(index);
    }

    if (wireframeItem && m_wireframeShader) {
        // Need dormant if not drawing active item
        wireframeItem->enable(!needActiveItem);

        // Set custom data for state overrides
        GizmoData* userData = dynamic_cast<GizmoData*>(wireframeItem->customData());
        if (!userData) {
            userData = new GizmoData();
        }
        if (userData) {
            userData->fHeight = fHeight;
            userData->fWidth = fWidth;
            userData->fRadius = fRadius;
            userData->fShapeType = fShapeType;
        }
        wireframeItem->setCustomData(userData);

        // Update the color and shader
        m_wireframeShader->setParameter(colorParameterName_, m_wireframeColor);
        wireframeItem->setShader(m_wireframeShader);
    }

    // 2. Add in a active wireframe render item
    //
    MHWRender::MRenderItem* activeWireframeItem = 0;
    index = list.indexOf(s_activeWireframeItemName);
    if (index < 0) {
        activeWireframeItem = MHWRender::MRenderItem::Create(
            s_activeWireframeItemName,
            MHWRender::MRenderItem::DecorationItem,
            MHWRender::MGeometry::kLines);

        activeWireframeItem->setDrawMode(MHWRender::MGeometry::kAll);
        activeWireframeItem->setSelectionMask(MSelectionMask::kSelectLights);
        activeWireframeItem->setWantConsolidation(true);
        activeWireframeItem->depthPriority(MHWRender::MRenderItem::sActiveLineDepthPriority);
        wireframeItem->setObjectTypeExclusionFlag(MHWRender::MFrameContext::kExcludeLights);
        list.append(activeWireframeItem);
    } else {
        activeWireframeItem = list.itemAt(index);
    }

    if (activeWireframeItem && m_activeWireframeShader) {
        // Enable if need active item
        activeWireframeItem->enable(needActiveItem);

        // Set custom data for state overrides
        GizmoData* userData = dynamic_cast<GizmoData*>(activeWireframeItem->customData());
        if (!userData) {
            userData = new GizmoData();
        }
        if (userData) {
            userData->fHeight = fHeight;
            userData->fWidth = fWidth;
            userData->fRadius = fRadius;
            userData->fShapeType = fShapeType;
        }
        activeWireframeItem->setCustomData(userData);

        // Update the color and shader
        m_activeWireframeShader->setParameter(colorParameterName_, m_wireframeColor);
        activeWireframeItem->setShader(m_activeWireframeShader);
    }
}

void GizmoGeometryOverride::populateGeometry(
    const MHWRender::MGeometryRequirements& requirements,
    const MHWRender::MRenderItemList&       renderItems,
    MHWRender::MGeometry&                   data)
{
    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer)
        return;

    const MHWRender::MShaderManager* shaderMgr = renderer->getShaderManager();
    if (!shaderMgr)
        return;

    // Build geometry if dirty
    if (m_geometryDirty) {
        double scale[3] = { m_scale[0], m_scale[1], m_scale[2] };
        double offset[3] = { m_offset[0], m_offset[1], m_offset[2] };
        switch (fShapeType) {
        case GizmoData::Capsule: break;
        case GizmoData::Circle: {
            DiskPrimitive primitiveData(offset, fRadius, true);
            m_wirePositions = primitiveData.wirePositions;
            m_wireIndexing = primitiveData.wireIndexing;

            LinePrimitive lineData;
            const int     lastVertexIndex = m_wirePositions.length();
            for (unsigned int i = 0; i < lineData.wirePositions.length(); i++) {
                m_wirePositions.append(lineData.wirePositions[i]);
            }
            for (unsigned int i = 0; i < lineData.wireIndexing.length(); i++) {
                m_wireIndexing.append(lastVertexIndex + lineData.wireIndexing[i]);
            }

            break;
        }
        case GizmoData::Cone: {
            ConePrimitive primitiveData(1.3f, fConeAngle, false);
            m_wirePositions = primitiveData.wirePositions;
            m_wireIndexing = primitiveData.wireIndexing;
            break;
        }
        case GizmoData::Cylinder: {
            CylinderPrimitive primitiveData(fRadius, fHeight);
            m_wirePositions = primitiveData.wirePositions;
            m_wireIndexing = primitiveData.wireIndexing;
            break;
        }
        case GizmoData::Distant: {
            double         arrowOffset[3] = { 0.45, 0.0, 1.0 };
            ArrowPrimitive arrow1Data(arrowOffset);

            m_wirePositions = arrow1Data.wirePositions;
            m_wireIndexing = arrow1Data.wireIndexing;

            arrowOffset[0] = -0.45;
            ArrowPrimitive arrow2Data(arrowOffset);
            int            lastVertexIndex = m_wirePositions.length();
            for (unsigned int i = 0; i < arrow2Data.wirePositions.length(); i++) {
                m_wirePositions.append(arrow2Data.wirePositions[i]);
            }
            for (unsigned int i = 0; i < arrow2Data.wireIndexing.length(); i++) {
                m_wireIndexing.append(lastVertexIndex + arrow2Data.wireIndexing[i]);
            }

            arrowOffset[0] = 0.0;
            arrowOffset[1] = 0.45;
            ArrowPrimitive arrow3Data(arrowOffset);
            lastVertexIndex = m_wirePositions.length();
            for (unsigned int i = 0; i < arrow3Data.wirePositions.length(); i++) {
                m_wirePositions.append(arrow3Data.wirePositions[i]);
            }
            for (unsigned int i = 0; i < arrow3Data.wireIndexing.length(); i++) {
                m_wireIndexing.append(lastVertexIndex + arrow3Data.wireIndexing[i]);
            }

            break;
        }
        case GizmoData::Quad: {
            double        scale[3] = { fWidth, fHeight, 1.0 };
            QuadPrimitive quadData(scale);
            m_wirePositions = quadData.wirePositions;
            m_wireIndexing = quadData.wireIndexing;

            LinePrimitive lineData;
            const int     lastVertexIndex = m_wirePositions.length();
            for (unsigned int i = 0; i < lineData.wirePositions.length(); i++) {
                m_wirePositions.append(lineData.wirePositions[i]);
            }
            for (unsigned int i = 0; i < lineData.wireIndexing.length(); i++) {
                m_wireIndexing.append(lastVertexIndex + lineData.wireIndexing[i]);
            }
            break;
        }
        case GizmoData::Sphere:
        case GizmoData::Dome: {
            SpherePrimitive primitiveData(fRadius, 8, scale, offset);
            m_wirePositions = primitiveData.wirePositions;
            m_wireIndexing = primitiveData.wireIndexing;
            break;
        }
        case GizmoData::Point:
        default: {
            SpherePrimitive primitiveData(0.1f, 4, scale, offset);
            m_wirePositions = primitiveData.wirePositions;
            m_wireIndexing = primitiveData.wireIndexing;
            break;
        }
        }

        m_geometryDirty = false;
    }

    // Wire vertex data
    MHWRender::MVertexBuffer* wirePositionBuffer = NULL;
    MHWRender::MIndexBuffer*  wireIndexBuffer = NULL;

    const MHWRender::MVertexBufferDescriptorList& listOfvertexBufforDescriptors
        = requirements.vertexRequirements();
    const int numberOfVertexRequirements = listOfvertexBufforDescriptors.length();

    MHWRender::MVertexBufferDescriptor vertexBufforDescriptor;
    for (int requirementNumber = 0; requirementNumber < numberOfVertexRequirements;
         requirementNumber++) {
        if (!listOfvertexBufforDescriptors.getDescriptor(
                requirementNumber, vertexBufforDescriptor)) {
            continue;
        }

        switch (vertexBufforDescriptor.semantic()) {
        case MHWRender::MGeometry::kPosition: {
            unsigned int totalVerts = m_wirePositions.length();
            if (totalVerts > 0) {
                wirePositionBuffer = data.createVertexBuffer(vertexBufforDescriptor);
                if (wirePositionBuffer) {
                    wirePositionBuffer->update(&m_wirePositions[0], 0, totalVerts, true);
                }
            }
            break;
        }
        case MHWRender::MGeometry::kNormal: {
            unsigned int totalVerts = m_wirePositions.length();
            if (totalVerts > 0) {
                wirePositionBuffer = data.createVertexBuffer(vertexBufforDescriptor);
                if (wirePositionBuffer) {
                    wirePositionBuffer->update(&m_wirePositions[0], 0, totalVerts, true);
                }
            }
            break;
        }
        default: {
            break;
        }
        }
    }

    // Update indexing for render items
    //
    int numItems = renderItems.length();
    for (int i = 0; i < numItems; i++) {
        const MHWRender::MRenderItem* item = renderItems.itemAt(i);
        if (!item) {
            continue;
        }
        if (item->name() == s_wireframeItemName || item->name() == s_activeWireframeItemName) {
            unsigned int indexCount = m_wireIndexing.length();
            if (indexCount) {
                wireIndexBuffer = data.createIndexBuffer(MHWRender::MGeometry::kUnsignedInt32);
                if (wireIndexBuffer) {
                    wireIndexBuffer->update(&m_wireIndexing[0], 0, indexCount, true);
                    item->associateWithIndexBuffer(wireIndexBuffer);
                }
            }
        }
    }
}
} // namespace MAYAUSD_NS_DEF