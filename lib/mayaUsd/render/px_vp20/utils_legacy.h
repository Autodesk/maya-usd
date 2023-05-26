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
#ifndef PXVP20_UTILS_LEGACY_H
#define PXVP20_UTILS_LEGACY_H

/// \file px_vp20/utils_legacy.h

#include <mayaUsd/base/api.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/types.h>

#include <maya/M3dView.h>
#include <maya/MFrameContext.h>
#include <maya/MSelectInfo.h>

PXR_NAMESPACE_OPEN_SCOPE

/// This class contains helper methods and utilities to help with the
/// transition from the Maya legacy viewport to Viewport 2.0.
class px_LegacyViewportUtils
{
public:
    /// Get the view and projection matrices used for selection from the
    /// given selection context in MSelectInfo \p selectInfo.
    MAYAUSD_CORE_PUBLIC
    static bool GetSelectionMatrices(
        MSelectInfo& selectInfo,
        GfMatrix4d&  viewMatrix,
        GfMatrix4d&  projectionMatrix);

    /// Helper function that converts M3dView::DisplayStyle from the legacy
    /// viewport into MHWRender::MFrameContext::DisplayStyle for Viewport
    /// 2.0.
    ///
    /// In the legacy viewport, the M3dView can be in exactly one
    /// displayStyle whereas Viewport 2.0's displayStyle is a bitmask of
    /// potentially multiple styles. To translate from the legacy viewport
    /// to Viewport 2.0, we simply bitwise-OR the single legacy viewport
    /// displayStyle into an empty mask.
    static unsigned int GetMFrameContextDisplayStyle(M3dView::DisplayStyle legacyDisplayStyle)
    {
        unsigned int displayStyle = 0u;

        switch (legacyDisplayStyle) {
        case M3dView::kBoundingBox:
            displayStyle |= MHWRender::MFrameContext::DisplayStyle::kBoundingBox;
            break;
        case M3dView::kFlatShaded:
            displayStyle |= MHWRender::MFrameContext::DisplayStyle::kFlatShaded;
            break;
        case M3dView::kGouraudShaded:
            displayStyle |= MHWRender::MFrameContext::DisplayStyle::kGouraudShaded;
            break;
        case M3dView::kWireFrame:
            displayStyle |= MHWRender::MFrameContext::DisplayStyle::kWireFrame;
            break;
        case M3dView::kPoints:
            // Not supported.
            break;
        }

        return displayStyle;
    }

    /// Returns true if the given Maya display style indicates that a
    /// bounding box should be rendered.
    static bool ShouldRenderBoundingBox(M3dView::DisplayStyle legacyDisplayStyle)
    {
        return (legacyDisplayStyle == M3dView::kBoundingBox);
    }

private:
    px_LegacyViewportUtils() = delete;
    ~px_LegacyViewportUtils() = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
