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
#include "pxr/pxr.h"
#include "pxrUsdMayaGL/hdImagingShapeUI.h"

#include "pxrUsdMayaGL/batchRenderer.h"
#include "pxrUsdMayaGL/debugCodes.h"
#include "pxrUsdMayaGL/instancerImager.h"
#include "pxrUsdMayaGL/userData.h"

#include "usdMaya/hdImagingShape.h"

#include "pxr/base/tf/debug.h"

#include <maya/M3dView.h>
#include <maya/MDagPath.h>
#include <maya/MDrawData.h>
#include <maya/MDrawInfo.h>
#include <maya/MDrawRequest.h>
#include <maya/MDrawRequestQueue.h>
#include <maya/MPxSurfaceShapeUI.h>


PXR_NAMESPACE_OPEN_SCOPE


/* static */
void*
PxrMayaHdImagingShapeUI::creator()
{
    UsdMayaGLBatchRenderer::Init();
    return new PxrMayaHdImagingShapeUI();
}

/* virtual */
void
PxrMayaHdImagingShapeUI::getDrawRequests(
        const MDrawInfo& drawInfo,
        bool /* objectAndActiveOnly */,
        MDrawRequestQueue& requests)
{
    const MDagPath shapeDagPath = drawInfo.multiPath();
    const PxrMayaHdImagingShape* imagingShape =
        PxrMayaHdImagingShape::GetShapeAtDagPath(shapeDagPath);
    if (!imagingShape) {
        return;
    }

    TF_DEBUG(PXRUSDMAYAGL_BATCHED_DRAWING).Msg(
        "PxrMayaHdImagingShapeUI::getDrawRequests(), shapeDagPath: %s\n",
        shapeDagPath.fullPathName().asChar());

    // Sync any instancers that need Hydra drawing.
    UsdMayaGL_InstancerImager::GetInstance().SyncShapeAdapters(
            drawInfo.displayStyle());

    // The legacy viewport never has an old MUserData we can reuse. It also
    // does not manage the data allocated in the MDrawData object, so the batch
    // renderer deletes the MUserData object at the end of a legacy viewport
    // Draw() call.
    PxrMayaHdUserData* userData = new PxrMayaHdUserData();
    userData->drawShape = true;

    MDrawData drawData;
    getDrawData(userData, drawData);

    MDrawRequest request = drawInfo.getPrototype(*this);
    request.setDrawData(drawData);

    requests.add(request);
}

/* virtual */
void
PxrMayaHdImagingShapeUI::draw(const MDrawRequest& request, M3dView& view) const
{
    TF_DEBUG(PXRUSDMAYAGL_BATCHED_DRAWING).Msg(
        "PxrMayaHdImagingShapeUI::draw()\n");

    view.beginGL();

    UsdMayaGLBatchRenderer::GetInstance().Draw(request, view);

    view.endGL();
}

PxrMayaHdImagingShapeUI::PxrMayaHdImagingShapeUI() : MPxSurfaceShapeUI()
{
}

/* virtual */
PxrMayaHdImagingShapeUI::~PxrMayaHdImagingShapeUI()
{
    UsdMayaGL_InstancerImager::GetInstance().RemoveShapeAdapters(/*vp2*/ false);
}


PXR_NAMESPACE_CLOSE_SCOPE
