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
#ifndef PXRUSDMAYAGL_HD_IMAGING_SHAPE_DRAW_OVERRIDE_H
#define PXRUSDMAYAGL_HD_IMAGING_SHAPE_DRAW_OVERRIDE_H

/// \file pxrUsdMayaGL/hdImagingShapeDrawOverride.h

#include "pxr/pxr.h"
#include "pxrUsdMayaGL/api.h"

#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MFrameContext.h>
#include <maya/MMatrix.h>
#include <maya/MObject.h>
#include <maya/MPxDrawOverride.h>
#include <maya/MString.h>
#include <maya/MUserData.h>
#include <maya/MViewport2Renderer.h>


PXR_NAMESPACE_OPEN_SCOPE


/// Draw override for drawing the pxrHdImagingShape node in Viewport 2.0
///
/// In most cases, there will only be a single instance of the
/// pxrHdImagingShape node in the scene, so this draw override will be the
/// thing that invokes the batch renderer to draw all Hydra-imaged Maya nodes.
///
/// Note that it does not support selection, so the individual nodes are still
/// responsible for managing that. We do, however, expect that this draw
/// override will cause Maya to issue a draw call with the "selectionPass"
/// semantic, which will provide a signal to the batch renderer that a
/// pick operation was attempted and that the next intersection test should
/// re-compute the selection.
class PxrMayaHdImagingShapeDrawOverride : public MHWRender::MPxDrawOverride
{
    public:

        PXRUSDMAYAGL_API
        static const MString drawDbClassification;

        PXRUSDMAYAGL_API
        static MHWRender::MPxDrawOverride* creator(const MObject& obj);

        PXRUSDMAYAGL_API
        ~PxrMayaHdImagingShapeDrawOverride() override;

        PXRUSDMAYAGL_API
        MHWRender::DrawAPI supportedDrawAPIs() const override;

        PXRUSDMAYAGL_API
        MMatrix transform(
                const MDagPath& objPath,
                const MDagPath& cameraPath) const override;

        PXRUSDMAYAGL_API
        MBoundingBox boundingBox(
                const MDagPath& objPath,
                const MDagPath& cameraPath) const override;

        PXRUSDMAYAGL_API
        bool isBounded(
                const MDagPath& objPath,
                const MDagPath& cameraPath) const override;

        PXRUSDMAYAGL_API
        bool disableInternalBoundingBoxDraw() const override;

        PXRUSDMAYAGL_API
        MUserData* prepareForDraw(
                const MDagPath& objPath,
                const MDagPath& cameraPath,
                const MHWRender::MFrameContext& frameContext,
                MUserData* oldData) override;

        PXRUSDMAYAGL_API
        static void draw(
                const MHWRender::MDrawContext& context,
                const MUserData* data);

    private:

        PxrMayaHdImagingShapeDrawOverride(const MObject& obj);

        PxrMayaHdImagingShapeDrawOverride(
                const PxrMayaHdImagingShapeDrawOverride&) = delete;
        PxrMayaHdImagingShapeDrawOverride& operator=(
                const PxrMayaHdImagingShapeDrawOverride&) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
