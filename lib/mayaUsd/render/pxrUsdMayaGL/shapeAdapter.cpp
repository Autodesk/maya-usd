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
#include "shapeAdapter.h"

#include <mayaUsd/base/api.h>
#include <mayaUsd/render/px_vp20/utils_legacy.h>
#include <mayaUsd/render/pxrUsdMayaGL/batchRenderer.h>
#include <mayaUsd/render/pxrUsdMayaGL/debugCodes.h>
#include <mayaUsd/render/pxrUsdMayaGL/renderParams.h>
#include <mayaUsd/render/pxrUsdMayaGL/softSelectHelper.h>
#include <mayaUsd/render/pxrUsdMayaGL/userData.h>

#include <pxr/base/gf/gamma.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/debug.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/imaging/hd/repr.h>
#include <pxr/imaging/hd/rprimCollection.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hd/types.h>
#include <pxr/imaging/hdx/tokens.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>

#include <maya/M3dView.h>
#include <maya/MBoundingBox.h>
#include <maya/MColor.h>
#include <maya/MDagPath.h>
#include <maya/MDrawData.h>
#include <maya/MDrawRequest.h>
#include <maya/MFnDagNode.h>
#include <maya/MFrameContext.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MObject.h>
#include <maya/MPxSurfaceShapeUI.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MUserData.h>
#include <maya/MUuid.h>

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Helper function that converts M3dView::DisplayStatus (legacy viewport) into
// MHWRender::DisplayStatus (Viewport 2.0).
static inline MHWRender::DisplayStatus
_ToMHWRenderDisplayStatus(const M3dView::DisplayStatus legacyDisplayStatus)
{
    // These enums are equivalent, but statically checking just in case.
    static_assert(
        ((int)M3dView::kActive == (int)MHWRender::kActive)
            && ((int)M3dView::kLive == (int)MHWRender::kLive)
            && ((int)M3dView::kDormant == (int)MHWRender::kDormant)
            && ((int)M3dView::kInvisible == (int)MHWRender::kInvisible)
            && ((int)M3dView::kHilite == (int)MHWRender::kHilite)
            && ((int)M3dView::kTemplate == (int)MHWRender::kTemplate)
            && ((int)M3dView::kActiveTemplate == (int)MHWRender::kActiveTemplate)
            && ((int)M3dView::kActiveComponent == (int)MHWRender::kActiveComponent)
            && ((int)M3dView::kLead == (int)MHWRender::kLead)
            && ((int)M3dView::kIntermediateObject == (int)MHWRender::kIntermediateObject)
            && ((int)M3dView::kActiveAffected == (int)MHWRender::kActiveAffected)
            && ((int)M3dView::kNoStatus == (int)MHWRender::kNoStatus),
        "M3dView::DisplayStatus == MHWRender::DisplayStatus");

    return MHWRender::DisplayStatus((int)legacyDisplayStatus);
}

static inline bool _IsActiveDisplayStatus(MHWRender::DisplayStatus displayStatus)
{
    return (displayStatus == MHWRender::DisplayStatus::kActive)
        || (displayStatus == MHWRender::DisplayStatus::kHilite)
        || (displayStatus == MHWRender::DisplayStatus::kActiveTemplate)
        || (displayStatus == MHWRender::DisplayStatus::kActiveComponent)
        || (displayStatus == MHWRender::DisplayStatus::kLead);
}

bool PxrMayaHdShapeAdapter::Sync(
    const MDagPath&              shapeDagPath,
    const M3dView::DisplayStyle  legacyDisplayStyle,
    const M3dView::DisplayStatus legacyDisplayStatus)
{
    // Legacy viewport implementation.

    UsdMayaGLBatchRenderer::GetInstance().StartBatchingFrameDiagnostics();

    const unsigned int displayStyle
        = px_LegacyViewportUtils::GetMFrameContextDisplayStyle(legacyDisplayStyle);
    const MHWRender::DisplayStatus displayStatus = _ToMHWRenderDisplayStatus(legacyDisplayStatus);

    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE)
        .Msg("Synchronizing PxrMayaHdShapeAdapter for legacy viewport: %p\n", this);

    const bool success = _Sync(shapeDagPath, displayStyle, displayStatus);

    if (success) {
        // The legacy viewport does not support color management, so we roll
        // our own gamma correction via framebuffer effect. But that means we
        // need to pre-linearize the wireframe color from Maya.
        //
        // The default value for wireframeColor is 0.0f for all four values and
        // if we need a wireframe color, we expect _Sync() to have set the
        // values and put 1.0f in for alpha, so inspect the alpha value to
        // determine whether we need to linearize rather than calling
        // _GetWireframeColor() again.
        if (_renderParams.wireframeColor[3] > 0.0f) {
            _renderParams.wireframeColor[3] = 1.0f;
            _renderParams.wireframeColor = GfConvertDisplayToLinear(_renderParams.wireframeColor);
        }
    }

    return success;
}

bool PxrMayaHdShapeAdapter::Sync(
    const MDagPath&                shapeDagPath,
    const unsigned int             displayStyle,
    const MHWRender::DisplayStatus displayStatus)
{
    // Viewport 2.0 implementation.

    UsdMayaGLBatchRenderer::GetInstance().StartBatchingFrameDiagnostics();

    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE)
        .Msg("Synchronizing PxrMayaHdShapeAdapter for Viewport 2.0: %p\n", this);

    return _Sync(shapeDagPath, displayStyle, displayStatus);
}

/* virtual */
bool PxrMayaHdShapeAdapter::UpdateVisibility(const M3dView* view) { return false; }

/* virtual */
bool PxrMayaHdShapeAdapter::IsVisible() const { return false; }

/* virtual */
void PxrMayaHdShapeAdapter::GetMayaUserData(
    MPxSurfaceShapeUI*  shapeUI,
    MDrawRequest&       drawRequest,
    const MBoundingBox* boundingBox)
{
    // Legacy viewport implementation.

    // The legacy viewport never has an old MUserData we can reuse.
    MUserData* userData = GetMayaUserData(nullptr, boundingBox);

    // Note that the legacy viewport does not manage the data allocated in the
    // MDrawData object, so the batch renderer deletes the MUserData object at
    // the end of a legacy viewport Draw() call.
    MDrawData drawData;
    shapeUI->getDrawData(userData, drawData);

    drawRequest.setDrawData(drawData);
}

/* virtual */
PxrMayaHdUserData*
PxrMayaHdShapeAdapter::GetMayaUserData(MUserData* oldData, const MBoundingBox* boundingBox)
{
    // Viewport 2.0 implementation (also called by legacy viewport
    // implementation).
    //
    // In the Viewport 2.0 prepareForDraw() usage, any MUserData object passed
    // into the function will be deleted by Maya. In the legacy viewport usage,
    // the object gets deleted at the end of a legacy viewport Draw() call.

    PxrMayaHdUserData* newData = dynamic_cast<PxrMayaHdUserData*>(oldData);
    if (!newData) {
        newData = new PxrMayaHdUserData();
    }

    if (boundingBox) {
        newData->boundingBox.reset(new MBoundingBox(*boundingBox));
        newData->wireframeColor.reset(new GfVec4f(_renderParams.wireframeColor));
    } else {
        newData->boundingBox.reset();
        newData->wireframeColor.reset();
    }

    return newData;
}

HdReprSelector
PxrMayaHdShapeAdapter::GetReprSelectorForDisplayStyle(unsigned int displayStyle) const
{
    HdReprSelector reprSelector;

    const bool boundingBoxStyle
        = displayStyle & MHWRender::MFrameContext::DisplayStyle::kBoundingBox;

    if (boundingBoxStyle) {
        // We don't currently use Hydra to draw bounding boxes, so we return an
        // empty repr selector here. Also, Maya seems to ignore most other
        // DisplayStyle bits when the viewport is in the kBoundingBox display
        // style anyway, and it just changes the color of the bounding box on
        // selection rather than adding in the wireframe like it does for
        // shaded display styles. So if we eventually do end up using Hydra for
        // bounding boxes, we could just return the appropriate repr here.
        return reprSelector;
    }

    const MHWRender::DisplayStatus displayStatus
        = MHWRender::MGeometryUtilities::displayStatus(_shapeDagPath);

    const bool isActive = _IsActiveDisplayStatus(displayStatus);

    const bool shadeActiveOnlyStyle
        = displayStyle & MHWRender::MFrameContext::DisplayStyle::kShadeActiveOnly;

    const bool wireframeStyle = (displayStyle & MHWRender::MFrameContext::DisplayStyle::kWireFrame)
        || _renderParams.useWireframe;

    const bool flatShadedStyle = displayStyle & MHWRender::MFrameContext::DisplayStyle::kFlatShaded;

    if (flatShadedStyle) {
        if (!shadeActiveOnlyStyle || isActive) {
            if (wireframeStyle) {
                reprSelector = HdReprSelector(HdReprTokens->wireOnSurf);
            } else {
                reprSelector = HdReprSelector(HdReprTokens->hull);
            }
        } else {
            // We're in shadeActiveOnly mode but this shape is not active.
            reprSelector = HdReprSelector(HdReprTokens->wire);
        }
    } else if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kGouraudShaded) {
        if (!shadeActiveOnlyStyle || isActive) {
            if (wireframeStyle) {
                reprSelector = HdReprSelector(HdReprTokens->refinedWireOnSurf);
            } else {
                reprSelector = HdReprSelector(HdReprTokens->refined);
            }
        } else {
            // We're in shadeActiveOnly mode but this shape is not active.
            reprSelector = HdReprSelector(HdReprTokens->refinedWire);
        }
    } else if (wireframeStyle) {
        reprSelector = HdReprSelector(HdReprTokens->refinedWire);
    } else if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kTwoSidedLighting) {
        // The UV editor uses the kTwoSidedLighting displayStyle.
        //
        // For now, to prevent objects from completely disappearing, we just
        // treat it similarly to kGouraudShaded.
        reprSelector = HdReprSelector(HdReprTokens->refined);
    }

    return reprSelector;
}

MStatus PxrMayaHdShapeAdapter::_SetDagPath(const MDagPath& dagPath)
{
    if (_shapeDagPath == dagPath) {
        return MS::kSuccess;
    }

    _shapeDagPath = dagPath;

    _shapeIdentifier = TfToken();
    _delegateId = SdfPath();

    _rprimCollectionMap.clear();
    _renderTaskIdMap.clear();

    MStatus    status;
    const bool dagPathIsValid = _shapeDagPath.isValid(&status);
    if (status != MS::kSuccess || !dagPathIsValid) {
        return status;
    }

    const MFnDagNode dagNodeFn(_shapeDagPath, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // We use the shape's UUID as a unique identifier for this shape adapter in
    // the batch renderer's SdfPath hierarchy. Since MPxDrawOverrides may be
    // constructed and destructed over the course of a node's lifetime (e.g. by
    // doing 'ogs -reset'), we need an identifier tied to the node's lifetime
    // rather than to the shape adapter's lifetime.
    const MUuid shapeUuid = dagNodeFn.uuid(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    const std::string uuidString(shapeUuid.asString().asChar(), shapeUuid.asString().length());

    _shapeIdentifier = TfToken(TfMakeValidIdentifier(uuidString));

    const UsdMayaGLBatchRenderer& batchRenderer = UsdMayaGLBatchRenderer::GetInstance();
    HdRenderIndex*                renderIndex = batchRenderer.GetRenderIndex();

    const SdfPath delegatePrefix = batchRenderer.GetDelegatePrefix(_isViewport2);

    _delegateId = delegatePrefix.AppendChild(_shapeIdentifier);

    // Create entries in the collection and render task ID maps for each repr
    // we might use.
    static const std::vector<HdReprSelector> allMayaReprs(
        { HdReprSelector(HdReprTokens->hull),
          HdReprSelector(HdReprTokens->refined),
          HdReprSelector(HdReprTokens->refinedWire),
          HdReprSelector(HdReprTokens->refinedWireOnSurf),
          HdReprSelector(HdReprTokens->wire),
          HdReprSelector(HdReprTokens->wireOnSurf) });

    for (const HdReprSelector& reprSelector : allMayaReprs) {
        const TfToken rprimCollectionName
            = TfToken(TfStringPrintf("%s_%s", _shapeIdentifier.GetText(), reprSelector.GetText()));
        _rprimCollectionMap.emplace(std::make_pair(
            reprSelector, HdRprimCollection(rprimCollectionName, reprSelector, _delegateId)));

        renderIndex->GetChangeTracker().AddCollection(rprimCollectionName);

        // We only generate the render task ID here. We'll leave it to the
        // batch renderer to lazily create the task in the render index when
        // it is first needed.
        const TfToken renderTaskName = TfToken(TfStringPrintf(
            "%s_%s", HdxPrimitiveTokens->renderTask.GetText(), rprimCollectionName.GetText()));
        _renderTaskIdMap.emplace(
            std::make_pair(reprSelector, _delegateId.AppendChild(renderTaskName)));
    }

    return status;
}

void PxrMayaHdShapeAdapter::_MarkRenderTasksDirty(HdDirtyBits dirtyBits)
{
    HdRenderIndex* renderIndex = UsdMayaGLBatchRenderer::GetInstance().GetRenderIndex();

    for (const auto& iter : _renderTaskIdMap) {
        // The render tasks represented by the IDs in this map are instantiated
        // lazily, so check that the task exists before attempting to dirty it.
        if (renderIndex->HasTask(iter.second)) {
            renderIndex->GetChangeTracker().MarkTaskDirty(iter.second, dirtyBits);
        }
    }
}

/* static */
bool PxrMayaHdShapeAdapter::_GetWireframeColor(
    MHWRender::DisplayStatus displayStatus,
    const MDagPath&          shapeDagPath,
    GfVec4f*                 wireframeColor)
{
    bool useWireframeColor = false;

    MColor mayaWireframeColor;

    // Dormant objects may be included in a soft selection.
    if (displayStatus == MHWRender::kDormant) {
        auto& batchRenderer = UsdMayaGLBatchRenderer::GetInstance();
        if (batchRenderer.GetObjectSoftSelectEnabled()) {
            const UsdMayaGLSoftSelectHelper& softSelectHelper
                = UsdMayaGLBatchRenderer::GetInstance().GetSoftSelectHelper();
            useWireframeColor = softSelectHelper.GetFalloffColor(shapeDagPath, &mayaWireframeColor);
        }
    }

    if (wireframeColor != nullptr) {
        // The caller wants a color returned. If the object isn't included in a
        // soft selection, just ask Maya for the wireframe color.
        if (!useWireframeColor) {
            mayaWireframeColor = MHWRender::MGeometryUtilities::wireframeColor(shapeDagPath);
        }

        *wireframeColor = GfVec4f(
            mayaWireframeColor.r, mayaWireframeColor.g, mayaWireframeColor.b, mayaWireframeColor.a);
    }

    if (_IsActiveDisplayStatus(displayStatus)) {
        useWireframeColor = true;
    }

    return useWireframeColor;
}

/* static */
bool PxrMayaHdShapeAdapter::_GetVisibility(
    const MDagPath& dagPath,
    const M3dView*  view,
    bool*           visibility)
{
    MStatus                        status;
    const MHWRender::DisplayStatus displayStatus
        = MHWRender::MGeometryUtilities::displayStatus(dagPath, &status);
    if (status != MS::kSuccess) {
        return false;
    }
    if (displayStatus == MHWRender::kInvisible) {
        *visibility = false;
        return true;
    }

    // The displayStatus() method above does not account for things like
    // display layers, so we also check the shape's dag path for its visibility
    // state.
    const bool dagPathIsVisible = dagPath.isVisible(&status);
    if (status != MS::kSuccess) {
        return false;
    }
    if (!dagPathIsVisible) {
        *visibility = false;
        return true;
    }

    // If a view was provided, check to see whether it is being filtered, and
    // get its isolated objects if so.
    MSelectionList isolatedObjects;
    if (view && view->viewIsFiltered()) {
        view->filteredObjectList(isolatedObjects);
    }

    // If non-empty, isolatedObjects contains the "root" isolated objects, so
    // we'll need to check to see if one of our ancestors was isolated. (The
    // ancestor check is potentially slow if you're isolating selection in
    // a very large scene.)
    // If empty, nothing is being isolated. (You don't pay the cost of any
    // ancestor checking in this case.)
    const bool somethingIsolated = !isolatedObjects.isEmpty(&status);
    if (status != MS::kSuccess) {
        return false;
    }
    if (somethingIsolated) {
        bool     isIsolateVisible = false;
        MDagPath curPath(dagPath);
        while (curPath.length()) {
            const bool hasItem = isolatedObjects.hasItem(curPath, MObject::kNullObj, &status);
            if (status != MS::kSuccess) {
                return false;
            }
            if (hasItem) {
                isIsolateVisible = true;
                break;
            }
            curPath.pop();
        }
        *visibility = isIsolateVisible;
        return true;
    }

    // Passed all visibility checks.
    *visibility = true;
    return true;
}

PxrMayaHdShapeAdapter::PxrMayaHdShapeAdapter(bool isViewport2)
    : _isViewport2(isViewport2)
{
    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE)
        .Msg("Constructing PxrMayaHdShapeAdapter: %p\n", this);
}

/* virtual */
PxrMayaHdShapeAdapter::~PxrMayaHdShapeAdapter()
{
    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE)
        .Msg("Destructing PxrMayaHdShapeAdapter: %p\n", this);
}

PXR_NAMESPACE_CLOSE_SCOPE
