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
#include "hdImagingShapeUI.h"

#include <mayaUsd/nodes/hdImagingShape.h>
#include <mayaUsd/render/pxrUsdMayaGL/batchRenderer.h>
#include <mayaUsd/render/pxrUsdMayaGL/debugCodes.h>
#include <mayaUsd/render/pxrUsdMayaGL/instancerImager.h>
#include <mayaUsd/render/pxrUsdMayaGL/userData.h>

#include <pxr/base/gf/vec2i.h>
#include <pxr/base/tf/debug.h>
#include <pxr/base/trace/trace.h>
#include <pxr/pxr.h>

#include <maya/M3dView.h>
#include <maya/MDGContext.h>
#include <maya/MDagPath.h>
#include <maya/MDrawData.h>
#include <maya/MDrawInfo.h>
#include <maya/MDrawRequest.h>
#include <maya/MDrawRequestQueue.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MProfiler.h>
#include <maya/MPxSurfaceShapeUI.h>
#include <maya/MStatus.h>

PXR_NAMESPACE_OPEN_SCOPE

/* static */
void* PxrMayaHdImagingShapeUI::creator()
{
    UsdMayaGLBatchRenderer::Init();
    return new PxrMayaHdImagingShapeUI();
}

/* virtual */
void PxrMayaHdImagingShapeUI::getDrawRequests(
    const MDrawInfo& drawInfo,
    bool /* objectAndActiveOnly */,
    MDrawRequestQueue& requests)
{
    TRACE_FUNCTION();

    MProfilingScope profilingScope(
        UsdMayaGLBatchRenderer::ProfilerCategory,
        MProfiler::kColorE_L2,
        "Hydra Imaging Shape getDrawRequests() (Legacy Viewport)");

    const MDagPath               shapeDagPath = drawInfo.multiPath();
    const PxrMayaHdImagingShape* imagingShape
        = PxrMayaHdImagingShape::GetShapeAtDagPath(shapeDagPath);
    if (!imagingShape) {
        return;
    }

    TF_DEBUG(PXRUSDMAYAGL_BATCHED_DRAWING)
        .Msg(
            "PxrMayaHdImagingShapeUI::getDrawRequests(), shapeDagPath: %s\n",
            shapeDagPath.fullPathName().asChar());

    // Grab batch renderer settings values from the shape here and pass them
    // along to the batch renderer. Settings that affect selection should then
    // be set appropriately for subsequent selections.
    MStatus                 status;
    const MFnDependencyNode depNodeFn(imagingShape->thisMObject(), &status);
    if (status == MS::kSuccess) {
        const MPlug selectionResolutionPlug
            = depNodeFn.findPlug(PxrMayaHdImagingShape::selectionResolutionAttr, &status);
        if (status == MS::kSuccess) {
            const short selectionResolution = selectionResolutionPlug.asShort(&status);
            if (status == MS::kSuccess) {
                UsdMayaGLBatchRenderer::GetInstance().SetSelectionResolution(
                    GfVec2i(selectionResolution));
            }
        }

        const MPlug enableDepthSelectionPlug
            = depNodeFn.findPlug(PxrMayaHdImagingShape::enableDepthSelectionAttr, &status);
        if (status == MS::kSuccess) {
            const bool enableDepthSelection = enableDepthSelectionPlug.asBool(&status);
            if (status == MS::kSuccess) {
                UsdMayaGLBatchRenderer::GetInstance().SetDepthSelectionEnabled(
                    enableDepthSelection);
            }
        }
    }

    // Sync any instancers that need Hydra drawing.
    UsdMayaGL_InstancerImager::GetInstance().SyncShapeAdapters(drawInfo.displayStyle());

    // The legacy viewport never has an old MUserData we can reuse. It also
    // does not manage the data allocated in the MDrawData object, so the batch
    // renderer deletes the MUserData object at the end of a legacy viewport
    // Draw() call.
    PxrMayaHdUserData* userData = new PxrMayaHdUserData();

    MDrawData drawData;
    getDrawData(userData, drawData);

    MDrawRequest request = drawInfo.getPrototype(*this);
    request.setDrawData(drawData);

    requests.add(request);
}

/* virtual */
void PxrMayaHdImagingShapeUI::draw(const MDrawRequest& request, M3dView& view) const
{
    TRACE_FUNCTION();

    MProfilingScope profilingScope(
        UsdMayaGLBatchRenderer::ProfilerCategory,
        MProfiler::kColorC_L1,
        "Hydra Imaging Shape draw() (Legacy Viewport)");

    TF_DEBUG(PXRUSDMAYAGL_BATCHED_DRAWING).Msg("PxrMayaHdImagingShapeUI::draw()\n");

    view.beginGL();

    UsdMayaGLBatchRenderer::GetInstance().Draw(request, view);

    view.endGL();
}

PxrMayaHdImagingShapeUI::PxrMayaHdImagingShapeUI()
    : MPxSurfaceShapeUI()
{
}

/* virtual */
PxrMayaHdImagingShapeUI::~PxrMayaHdImagingShapeUI()
{
    UsdMayaGL_InstancerImager::GetInstance().RemoveShapeAdapters(/*vp2*/ false);
}

PXR_NAMESPACE_CLOSE_SCOPE
