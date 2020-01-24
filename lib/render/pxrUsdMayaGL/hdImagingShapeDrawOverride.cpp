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
#include "./hdImagingShapeDrawOverride.h"

#include "./batchRenderer.h"
#include "./debugCodes.h"
#include "./instancerImager.h"
#include "./userData.h"

#include "../../nodes/hdImagingShape.h"

#include "pxr/base/gf/vec2i.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/stringUtils.h"

#include <maya/MBoundingBox.h>
#include <maya/MDGContext.h>
#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFrameContext.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPxDrawOverride.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MUserData.h>
#include <maya/MViewport2Renderer.h>


PXR_NAMESPACE_OPEN_SCOPE


const MString PxrMayaHdImagingShapeDrawOverride::drawDbClassification(
    TfStringPrintf("drawdb/geometry/pxrUsdMayaGL/%s",
                   PxrMayaHdImagingShapeTokens->MayaTypeName.GetText()).c_str());

/* static */
MHWRender::MPxDrawOverride*
PxrMayaHdImagingShapeDrawOverride::creator(const MObject& obj)
{
    UsdMayaGLBatchRenderer::Init();
    return new PxrMayaHdImagingShapeDrawOverride(obj);
}

/* virtual */
PxrMayaHdImagingShapeDrawOverride::~PxrMayaHdImagingShapeDrawOverride()
{
    UsdMayaGL_InstancerImager::GetInstance().RemoveShapeAdapters(/*vp2*/ true);
}

/* virtual */
MHWRender::DrawAPI
PxrMayaHdImagingShapeDrawOverride::supportedDrawAPIs() const
{
#if MAYA_API_VERSION >= 201600
    return MHWRender::kOpenGL | MHWRender::kOpenGLCoreProfile;
#else
    return MHWRender::kOpenGL;
#endif
}

/* virtual */
MMatrix
PxrMayaHdImagingShapeDrawOverride::transform(
        const MDagPath& /* objPath */,
        const MDagPath& /* cameraPath */) const
{
    // Always ignore any transform on the pxrHdImagingShape and use an identity
    // transform instead.
    return MMatrix();
}

/* virtual */
MBoundingBox
PxrMayaHdImagingShapeDrawOverride::boundingBox(
        const MDagPath& objPath,
        const MDagPath& /* cameraPath */) const
{
    const PxrMayaHdImagingShape* imagingShape =
        PxrMayaHdImagingShape::GetShapeAtDagPath(objPath);
    if (!imagingShape) {
        return MBoundingBox();
    }

    return imagingShape->boundingBox();
}

/* virtual */
bool
PxrMayaHdImagingShapeDrawOverride::isBounded(
        const MDagPath& objPath,
        const MDagPath& /* cameraPath */) const
{
    const PxrMayaHdImagingShape* imagingShape =
        PxrMayaHdImagingShape::GetShapeAtDagPath(objPath);
    if (!imagingShape) {
        return false;
    }

    return imagingShape->isBounded();
}

/* virtual */
bool
PxrMayaHdImagingShapeDrawOverride::disableInternalBoundingBoxDraw() const
{
    return true;
}

/* virtual */
MUserData*
PxrMayaHdImagingShapeDrawOverride::prepareForDraw(
        const MDagPath& objPath,
        const MDagPath& /* cameraPath */,
        const MHWRender::MFrameContext& frameContext,
        MUserData* oldData)
{
    const PxrMayaHdImagingShape* imagingShape =
        PxrMayaHdImagingShape::GetShapeAtDagPath(objPath);
    if (!imagingShape) {
        return nullptr;
    }

    TF_DEBUG(PXRUSDMAYAGL_BATCHED_DRAWING).Msg(
        "PxrMayaHdImagingShapeDrawOverride::prepareForDraw(), objPath: %s\n",
        objPath.fullPathName().asChar());

    // The HdImagingShape is very rarely marked dirty, but one of the things
    // that does so is changing batch renderer settings attributes, so we grab
    // the values from the shape here and pass them along to the batch
    // renderer. Settings that affect selection should then be set
    // appropriately for subsequent selections.
    MStatus status;
    const MFnDependencyNode depNodeFn(imagingShape->thisMObject(), &status);
    if (status == MS::kSuccess) {
        const MPlug selectionResolutionPlug =
            depNodeFn.findPlug(
                PxrMayaHdImagingShape::selectionResolutionAttr,
                &status);
        if (status == MS::kSuccess) {
            const short selectionResolution =
#if MAYA_API_VERSION >= 20180000
                selectionResolutionPlug.asShort(&status);
#else
                selectionResolutionPlug.asShort(MDGContext::fsNormal, &status);
#endif
            if (status == MS::kSuccess) {
                UsdMayaGLBatchRenderer::GetInstance().SetSelectionResolution(
                    GfVec2i(selectionResolution));
            }
        }

        const MPlug enableDepthSelectionPlug =
            depNodeFn.findPlug(
                PxrMayaHdImagingShape::enableDepthSelectionAttr,
                &status);
        if (status == MS::kSuccess) {
            const bool enableDepthSelection =
#if MAYA_API_VERSION >= 20180000
                enableDepthSelectionPlug.asBool(&status);
#else
                enableDepthSelectionPlug.asBool(MDGContext::fsNormal, &status);
#endif
            if (status == MS::kSuccess) {
                UsdMayaGLBatchRenderer::GetInstance().SetDepthSelectionEnabled(
                    enableDepthSelection);
            }
        }
    }

    // Sync any instancers that need Hydra drawing.
    UsdMayaGL_InstancerImager::GetInstance().SyncShapeAdapters(
            frameContext.getDisplayStyle());

    PxrMayaHdUserData* newData = dynamic_cast<PxrMayaHdUserData*>(oldData);
    if (!newData) {
        newData = new PxrMayaHdUserData();
    }

    newData->drawShape = true;

    return newData;
}

/* static */
void
PxrMayaHdImagingShapeDrawOverride::draw(
        const MHWRender::MDrawContext& context,
        const MUserData* data)
{
    TF_DEBUG(PXRUSDMAYAGL_BATCHED_DRAWING).Msg(
        "PxrMayaHdImagingShapeDrawOverride::draw()\n");

    UsdMayaGLBatchRenderer::GetInstance().Draw(context, data);
}

// Note that isAlwaysDirty became available as an MPxDrawOverride constructor
// parameter beginning with Maya 2016 Extension 2.
PxrMayaHdImagingShapeDrawOverride::PxrMayaHdImagingShapeDrawOverride(
        const MObject& obj) :
    MHWRender::MPxDrawOverride(obj,
                               PxrMayaHdImagingShapeDrawOverride::draw
#if MAYA_API_VERSION >= 201651
                               , /* isAlwaysDirty = */ false)
#else
                               )
#endif
{
}


PXR_NAMESPACE_CLOSE_SCOPE
