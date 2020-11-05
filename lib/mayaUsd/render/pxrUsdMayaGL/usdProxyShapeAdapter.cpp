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
#include "usdProxyShapeAdapter.h"

#include <mayaUsd/base/api.h>
#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/render/pxrUsdMayaGL/batchRenderer.h>
#include <mayaUsd/render/pxrUsdMayaGL/debugCodes.h>
#include <mayaUsd/render/pxrUsdMayaGL/renderParams.h>
#include <mayaUsd/render/pxrUsdMayaGL/shapeAdapter.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/tf/debug.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/trace/trace.h>
#include <pxr/imaging/hd/enums.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/repr.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <maya/M3dView.h>
#include <maya/MDagPath.h>
#include <maya/MFrameContext.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MMatrix.h>
#include <maya/MProfiler.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/* virtual */
bool PxrMayaHdUsdProxyShapeAdapter::UpdateVisibility(const M3dView* view)
{
    bool isVisible;

    /// Note that M3dView::pluginObjectDisplay() is unfortunately not declared
    /// const, so we have to cast away the const-ness here.
    M3dView* nonConstView = const_cast<M3dView*>(view);

    if (nonConstView
        && !nonConstView->pluginObjectDisplay(MayaUsdProxyShapeBase::displayFilterName)) {
        // USD proxy shapes are being filtered from this view, so don't bother
        // checking any other visibility state.
        isVisible = false;
    } else if (!_GetVisibility(GetDagPath(), view, &isVisible)) {
        return false;
    }

    if (_delegate && _delegate->GetRootVisibility() != isVisible) {
        _delegate->SetRootVisibility(isVisible);
        return true;
    }

    return false;
}

/* virtual */
bool PxrMayaHdUsdProxyShapeAdapter::IsVisible() const
{
    return (_delegate && _delegate->GetRootVisibility());
}

/* virtual */
void PxrMayaHdUsdProxyShapeAdapter::SetRootXform(const GfMatrix4d& transform)
{
    _rootXform = transform;

    if (_delegate) {
        _delegate->SetRootTransform(_rootXform);
    }
}

/* virtual */
bool PxrMayaHdUsdProxyShapeAdapter::_Sync(
    const MDagPath&                shapeDagPath,
    const unsigned int             displayStyle,
    const MHWRender::DisplayStatus displayStatus)
{
    TRACE_FUNCTION();

    MProfilingScope profilingScope(
        UsdMayaGLBatchRenderer::ProfilerCategory,
        MProfiler::kColorE_L2,
        "USD Proxy Shape Syncing Shape Adapter");

    MayaUsdProxyShapeBase* usdProxyShape = MayaUsdProxyShapeBase::GetShapeAtDagPath(shapeDagPath);
    if (!usdProxyShape) {
        TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE)
            .Msg(
                "Failed to get MayaUsdProxyShapeBase for '%s'\n",
                shapeDagPath.fullPathName().asChar());
        return false;
    }

    UsdPrim       usdPrim;
    SdfPathVector excludedPrimPaths;
    int           refineLevel;
    UsdTimeCode   timeCode;
    bool          drawRenderPurpose = false;
    bool          drawProxyPurpose = true;
    bool          drawGuidePurpose = false;
    if (!usdProxyShape->GetAllRenderAttributes(
            &usdPrim,
            &excludedPrimPaths,
            &refineLevel,
            &timeCode,
            &drawRenderPurpose,
            &drawProxyPurpose,
            &drawGuidePurpose)) {
        TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE)
            .Msg(
                "Failed to get render attributes for MayaUsdProxyShapeBase '%s'\n",
                shapeDagPath.fullPathName().asChar());
        return false;
    }

    // Check for updates to the shape or changes in the batch renderer that
    // require us to re-initialize the shape adapter.
    HdRenderIndex* renderIndex = UsdMayaGLBatchRenderer::GetInstance().GetRenderIndex();
    if (!(shapeDagPath == GetDagPath()) || usdPrim != _rootPrim
        || excludedPrimPaths != _excludedPrimPaths || !_delegate
        || renderIndex != &_delegate->GetRenderIndex()) {
        _SetDagPath(shapeDagPath);
        _rootPrim = usdPrim;
        _excludedPrimPaths = excludedPrimPaths;

        if (!_Init(renderIndex)) {
            return false;
        }
    }

    // Reset _renderParams to the defaults.
    PxrMayaHdRenderParams renderParams;
    _renderParams = renderParams;

    // Update Render Tags
    _renderTags.clear();
    _renderTags.push_back(HdRenderTagTokens->geometry);
    if (drawRenderPurpose) {
        _renderTags.push_back(HdRenderTagTokens->render);
    }
    if (drawProxyPurpose) {
        _renderTags.push_back(HdRenderTagTokens->proxy);
    }
    if (drawGuidePurpose) {
        _renderTags.push_back(HdRenderTagTokens->guide);
    }

    MStatus       status;
    const MMatrix transform = GetDagPath().inclusiveMatrix(&status);
    if (status == MS::kSuccess) {
        _rootXform = GfMatrix4d(transform.matrix);
        _delegate->SetRootTransform(_rootXform);
    }

    _delegate->SetRefineLevelFallback(refineLevel);

    // Will only react if time actually changes.
    _delegate->SetTime(timeCode);

    _renderParams.useWireframe
        = _GetWireframeColor(displayStatus, GetDagPath(), &_renderParams.wireframeColor);

    // XXX: This is not technically correct. Since the display style can vary
    // per viewport, this decision of whether or not to enable lighting should
    // be delayed until when the repr for each viewport is known during batched
    // drawing. For now, the incorrectly shaded wireframe is not too offensive
    // though.
    //
    // If the repr selector specifies a wireframe-only repr, then disable
    // lighting. The useWireframe property of the render params is used to
    // determine the repr, so be sure to do this *after* that has been set.
    const HdReprSelector reprSelector = GetReprSelectorForDisplayStyle(displayStyle);
    if (reprSelector.Contains(HdReprTokens->wire)
        || reprSelector.Contains(HdReprTokens->refinedWire)) {
        _renderParams.enableLighting = false;
    }

    HdCullStyle cullStyle = HdCullStyleNothing;
    if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kBackfaceCulling) {
        cullStyle = HdCullStyleBackUnlessDoubleSided;
    }

    _delegate->SetCullStyleFallback(cullStyle);

    return true;
}

bool PxrMayaHdUsdProxyShapeAdapter::_Init(HdRenderIndex* renderIndex)
{
    TRACE_FUNCTION();

    MProfilingScope profilingScope(
        UsdMayaGLBatchRenderer::ProfilerCategory,
        MProfiler::kColorE_L2,
        "USD Proxy Shape Initializing Shape Adapter");

    if (!TF_VERIFY(renderIndex, "Cannot initialize shape adapter with invalid HdRenderIndex")) {
        return false;
    }

    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE)
        .Msg(
            "Initializing PxrMayaHdUsdProxyShapeAdapter: %p\n"
            "    shape DAG path  : %s\n"
            "    shape identifier: %s\n"
            "    delegateId      : %s\n",
            this,
            GetDagPath().fullPathName().asChar(),
            _shapeIdentifier.GetText(),
            _delegateId.GetText());

    _delegate.reset(new UsdImagingDelegate(renderIndex, _delegateId));
    if (!TF_VERIFY(
            _delegate,
            "Failed to create shape adapter delegate for shape %s",
            GetDagPath().fullPathName().asChar())) {
        return false;
    }

    if (TfDebug::IsEnabled(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE)) {
        TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE)
            .Msg(
                "    Populating delegate:\n"
                "        rootPrim         : %s\n"
                "        excludedPrimPaths: ",
                _rootPrim.GetPath().GetText());
        for (const SdfPath& primPath : _excludedPrimPaths) {
            TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg("%s ", primPath.GetText());
        }
        TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg("\n");
    }

    _delegate->Populate(_rootPrim, _excludedPrimPaths, SdfPathVector());

    return true;
}

PxrMayaHdUsdProxyShapeAdapter::PxrMayaHdUsdProxyShapeAdapter(bool isViewport2)
    : PxrMayaHdShapeAdapter(isViewport2)
{
    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE)
        .Msg("Constructing PxrMayaHdUsdProxyShapeAdapter: %p\n", this);
}

/* virtual */
PxrMayaHdUsdProxyShapeAdapter::~PxrMayaHdUsdProxyShapeAdapter()
{
    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE)
        .Msg("Destructing PxrMayaHdUsdProxyShapeAdapter: %p\n", this);
}

PXR_NAMESPACE_CLOSE_SCOPE
