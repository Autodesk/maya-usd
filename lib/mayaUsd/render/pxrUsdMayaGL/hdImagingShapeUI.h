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
#ifndef PXRUSDMAYAGL_HD_IMAGING_SHAPE_UI_H
#define PXRUSDMAYAGL_HD_IMAGING_SHAPE_UI_H

/// \file pxrUsdMayaGL/hdImagingShapeUI.h
#include <pxr/pxr.h>
// XXX: With Maya versions up through 2019 on Linux, M3dView.h ends up
// indirectly including an X11 header that #define's "Bool" as int:
//   - <maya/M3dView.h> includes <maya/MNativeWindowHdl.h>
//   - <maya/MNativeWindowHdl.h> includes <X11/Intrinsic.h>
//   - <X11/Intrinsic.h> includes <X11/Xlib.h>
//   - <X11/Xlib.h> does: "#define Bool int"
// This can cause compilation issues if <pxr/usd/sdf/types.h> is included
// afterwards, so to fix this, we ensure that it gets included first.
//
// The X11 include appears to have been removed in Maya 2020+, so this should
// no longer be an issue with later versions.
#include <mayaUsd/base/api.h>

#include <pxr/usd/sdf/types.h>

#include <maya/M3dView.h>
#include <maya/MDrawInfo.h>
#include <maya/MDrawRequest.h>
#include <maya/MDrawRequestQueue.h>
#include <maya/MPxSurfaceShapeUI.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Class for drawing the pxrHdImagingShape node in the legacy viewport
///
/// In most cases, there will only be a single instance of the
/// pxrHdImagingShape node in the scene, so this class will be the thing that
/// invokes the batch renderer to draw all Hydra-imaged Maya nodes.
///
/// Note that it does not support selection, so the individual nodes are still
/// responsible for managing that.
class PxrMayaHdImagingShapeUI : public MPxSurfaceShapeUI
{
public:
    MAYAUSD_CORE_PUBLIC
    static void* creator();

    MAYAUSD_CORE_PUBLIC
    void getDrawRequests(
        const MDrawInfo&   drawInfo,
        bool               objectAndActiveOnly,
        MDrawRequestQueue& requests) override;

    MAYAUSD_CORE_PUBLIC
    void draw(const MDrawRequest& request, M3dView& view) const override;

private:
    PxrMayaHdImagingShapeUI();
    ~PxrMayaHdImagingShapeUI() override;

    PxrMayaHdImagingShapeUI(const PxrMayaHdImagingShapeUI&) = delete;
    PxrMayaHdImagingShapeUI& operator=(const PxrMayaHdImagingShapeUI&) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
