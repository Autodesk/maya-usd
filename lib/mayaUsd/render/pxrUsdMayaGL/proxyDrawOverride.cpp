//
// Copyright 2016 Pixar
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
#include "proxyDrawOverride.h"

#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/render/px_vp20/utils.h>
#include <mayaUsd/render/pxrUsdMayaGL/batchRenderer.h>
#include <mayaUsd/render/pxrUsdMayaGL/renderParams.h>
#include <mayaUsd/render/pxrUsdMayaGL/usdProxyShapeAdapter.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/trace/trace.h>
#include <pxr/pxr.h>

#include <maya/M3dView.h>
#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MFrameContext.h>
#include <maya/MMatrix.h>
#include <maya/MObject.h>
#include <maya/MPoint.h>
#include <maya/MProfiler.h>
#include <maya/MPxDrawOverride.h>
#include <maya/MSelectionContext.h>
#include <maya/MSelectionMask.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MUserData.h>
#include <maya/MViewport2Renderer.h>

PXR_NAMESPACE_OPEN_SCOPE

const MString UsdMayaProxyDrawOverride::drawDbClassification(
    TfStringPrintf(
        "drawdb/geometry/pxrUsdMayaGL/%s",
        MayaUsdProxyShapeBaseTokens->MayaTypeName.GetText())
        .c_str());

/* static */
MHWRender::MPxDrawOverride* UsdMayaProxyDrawOverride::Creator(const MObject& obj)
{
    UsdMayaGLBatchRenderer::Init();
    return new UsdMayaProxyDrawOverride(obj);
}

UsdMayaProxyDrawOverride::UsdMayaProxyDrawOverride(const MObject& obj)
    : MHWRender::MPxDrawOverride(
        obj,
        UsdMayaProxyDrawOverride::draw,
        /* isAlwaysDirty = */ false)
    , _shapeAdapter(/* isViewport2 = */ true)
{
}

/* virtual */
UsdMayaProxyDrawOverride::~UsdMayaProxyDrawOverride()
{
    UsdMayaGLBatchRenderer::GetInstance().RemoveShapeAdapter(&_shapeAdapter);
}

/* virtual */
MHWRender::DrawAPI UsdMayaProxyDrawOverride::supportedDrawAPIs() const
{
    return MHWRender::kOpenGL | MHWRender::kOpenGLCoreProfile;
}

/* virtual */
MMatrix
UsdMayaProxyDrawOverride::transform(const MDagPath& objPath, const MDagPath& cameraPath) const
{
    // Propagate changes in the proxy shape's transform to the shape adapter's
    // delegate.
    MStatus       status;
    const MMatrix transform = objPath.inclusiveMatrix(&status);
    if (status == MS::kSuccess) {
        const_cast<PxrMayaHdUsdProxyShapeAdapter&>(_shapeAdapter)
            .SetRootXform(GfMatrix4d(transform.matrix));
    }

    return MHWRender::MPxDrawOverride::transform(objPath, cameraPath);
}

/* virtual */
MBoundingBox UsdMayaProxyDrawOverride::boundingBox(
    const MDagPath& objPath,
    const MDagPath& /* cameraPath */) const
{
    TRACE_FUNCTION();

    MProfilingScope profilingScope(
        UsdMayaGLBatchRenderer::ProfilerCategory,
        MProfiler::kColorE_L1,
        "USD Proxy Shape Computing Bounding Box (Viewport 2.0)");

    // If a proxy shape is connected to a Maya instancer, a draw override will
    // be generated for the proxy shape, but callbacks will get the instancer
    // DAG path instead. Since we properly handle instancers using the
    // UsdMayaGL_InstancerImager, silently ignore this weird case.
    if (objPath.apiType() == MFn::kInstancer) {
        return MBoundingBox();
    }

    MayaUsdProxyShapeBase* pShape = MayaUsdProxyShapeBase::GetShapeAtDagPath(objPath);
    if (!pShape) {
        return MBoundingBox();
    }

    return pShape->boundingBox();
}

/* virtual */
bool UsdMayaProxyDrawOverride::isBounded(const MDagPath& objPath, const MDagPath& /* cameraPath */)
    const
{
    // XXX: Ideally, we'd be querying the shape itself using the code below to
    // determine whether the object is bounded or not. Unfortunately, the
    // shape's bounded-ness is based on the PIXMAYA_ENABLE_BOUNDING_BOX_MODE
    // environment variable, which is almost never enabled. This is because we
    // want Maya to bypass its own costly CPU-based frustum culling in favor
    // of Hydra's higher performance implementation.
    // However, this causes problems for features in Viewport 2.0 such as
    // automatic computation of width focus for directional lights since it
    // cannot get a bounding box for the shape.
    // It would be preferable to just remove the use of
    // PIXMAYA_ENABLE_BOUNDING_BOX_MODE in the shape's isBounded() method,
    // especially since we instruct Maya not to draw bounding boxes in
    // disableInternalBoundingBoxDraw() below, but trying to do that caused
    // performance degradation in selection.
    // So rather than ask the shape whether it is bounded or not, the draw
    // override simply *always* considers the shape bounded. Hopefully at some
    // point we can get Maya to fully bypass all of its frustum culling and
    // remove PIXMAYA_ENABLE_BOUNDING_BOX_MODE.
    return true;

    // MayaUsdProxyShapeBase* pShape = MayaUsdProxyShapeBase::GetShapeAtDagPath(objPath);
    // if (!pShape) {
    //     return false;
    // }

    // return pShape->isBounded();
}

/* virtual */
bool UsdMayaProxyDrawOverride::disableInternalBoundingBoxDraw() const
{
    // Hydra performs its own high-performance frustum culling, so we don't
    // want to rely on Maya to do it on the CPU. As such, the best performance
    // comes from telling Maya *not* to draw bounding boxes.
    return true;
}

/* virtual */
MUserData* UsdMayaProxyDrawOverride::prepareForDraw(
    const MDagPath& objPath,
    const MDagPath& /* cameraPath */,
    const MHWRender::MFrameContext& frameContext,
    MUserData*                      oldData)
{
    TRACE_FUNCTION();

    MProfilingScope profilingScope(
        UsdMayaGLBatchRenderer::ProfilerCategory,
        MProfiler::kColorE_L2,
        "USD Proxy Shape prepareForDraw() (Viewport 2.0)");

    // If a proxy shape is connected to a Maya instancer, a draw override will
    // be generated for the proxy shape, but callbacks will get the instancer
    // DAG path instead. Since we properly handle instancer drawing in this
    // library (using the pxrHdImagingShape), we can safely ignore this case.
    if (objPath.apiType() == MFn::kInstancer) {
        return nullptr;
    }

    MayaUsdProxyShapeBase* shape = MayaUsdProxyShapeBase::GetShapeAtDagPath(objPath);
    if (!shape) {
        return nullptr;
    }

    if (!_shapeAdapter.Sync(
            objPath,
            frameContext.getDisplayStyle(),
            MHWRender::MGeometryUtilities::displayStatus(objPath))) {
        return nullptr;
    }

    UsdMayaGLBatchRenderer::GetInstance().AddShapeAdapter(&_shapeAdapter);

    const MBoundingBox boundingBox = shape->boundingBox();

    return _shapeAdapter.GetMayaUserData(oldData, &boundingBox);
}

/* virtual */
bool UsdMayaProxyDrawOverride::wantUserSelection() const
{
    const MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer) {
        return false;
    }

    return renderer->drawAPIIsOpenGL();
}

/* virtual */
bool UsdMayaProxyDrawOverride::userSelect(
    MHWRender::MSelectionInfo&     selectionInfo,
    const MHWRender::MDrawContext& context,
    MPoint&                        hitPoint,
    const MUserData*               data)
{
    TRACE_FUNCTION();

    MProfilingScope profilingScope(
        UsdMayaGLBatchRenderer::ProfilerCategory,
        MProfiler::kColorE_L2,
        "USD Proxy Shape userSelect() (Viewport 2.0)");

    M3dView    view;
    const bool hasView = px_vp20Utils::GetViewFromDrawContext(context, view);
    if (hasView && !view.pluginObjectDisplay(MayaUsdProxyShapeBase::displayFilterName)) {
        return false;
    }

    MSelectionMask objectsMask(MSelectionMask::kSelectObjectsMask);
    if (!selectionInfo.selectable(objectsMask)) {
        return false;
    }

    const unsigned int             displayStyle = context.getDisplayStyle();
    const MHWRender::DisplayStatus displayStatus
        = MHWRender::MGeometryUtilities::displayStatus(_shapeAdapter.GetDagPath());

    // At this point, we expect the shape to have already been drawn and our
    // shape adapter to have been added to the batch renderer, but just in
    // case, we still treat the shape adapter as if we're populating it for the
    // first time. We do not add it to the batch renderer though, since that
    // must have already been done to have caused the shape to be drawn and
    // become eligible for selection.
    if (!_shapeAdapter.Sync(_shapeAdapter.GetDagPath(), displayStyle, displayStatus)) {
        return false;
    }

    const HdxPickHitVector* hitSet = UsdMayaGLBatchRenderer::GetInstance().TestIntersection(
        &_shapeAdapter, selectionInfo, context);

    const HdxPickHit* nearestHit = UsdMayaGLBatchRenderer::GetNearestHit(hitSet);

    if (!nearestHit) {
        return false;
    }

    const GfVec3f& gfHitPoint = nearestHit->worldSpaceHitPoint;
    hitPoint = MPoint(gfHitPoint[0], gfHitPoint[1], gfHitPoint[2]);

    return true;
}

/* static */
void UsdMayaProxyDrawOverride::draw(const MHWRender::MDrawContext& context, const MUserData* data)
{
    TRACE_FUNCTION();

    MProfilingScope profilingScope(
        UsdMayaGLBatchRenderer::ProfilerCategory,
        MProfiler::kColorC_L1,
        "USD Proxy Shape draw() (Viewport 2.0)");

    const unsigned int displayStyle = context.getDisplayStyle();
    if (!px_vp20Utils::ShouldRenderBoundingBox(displayStyle)) {
        return;
    }

    UsdMayaGLBatchRenderer::GetInstance().DrawBoundingBox(context, data);
}

PXR_NAMESPACE_CLOSE_SCOPE
