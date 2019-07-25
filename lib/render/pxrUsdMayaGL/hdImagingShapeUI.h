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

#include "pxr/pxr.h"
#include "../../base/api.h"

// XXX: On Linux, some Maya headers (notably M3dView.h) end up indirectly
//      including X11/Xlib.h, which #define's "Bool" as int. This can cause
//      compilation issues if sdf/types.h is included afterwards, so to fix
//      this, we ensure that it gets included first.
#include "pxr/usd/sdf/types.h"

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
                const MDrawInfo& drawInfo,
                bool objectAndActiveOnly,
                MDrawRequestQueue& requests) override;

        MAYAUSD_CORE_PUBLIC
        void draw(const MDrawRequest& request, M3dView& view) const override;

    private:

        PxrMayaHdImagingShapeUI();
        ~PxrMayaHdImagingShapeUI() override;

        PxrMayaHdImagingShapeUI(const PxrMayaHdImagingShapeUI&) = delete;
        PxrMayaHdImagingShapeUI& operator=(
                const PxrMayaHdImagingShapeUI&) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
