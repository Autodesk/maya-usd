//
// Copyright 2026 Autodesk
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
#ifndef MAYAUSD_VP2_RENDER_PASS_PUBLISHER_H
#define MAYAUSD_VP2_RENDER_PASS_PUBLISHER_H

#include "renderPassSceneIndex.h"

#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/retainedSceneIndex.h>
#include <pxr/imaging/hdsi/sceneGlobalsSceneIndex.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

/// Bridges a proxy shape's active render pass into the Hydra scene feeding VP2.
///
/// VP2 populates via UsdImagingDelegate, which emits no Hydra prim for a
/// non-imageable UsdRenderPass, so MayaUsdRenderPassSceneIndex would never find
/// a pass to apply. Rather than image the stage a second time, this reads the
/// pass's collections off the USD stage and republishes them as a synthetic
/// prim in a small retained scene index, which the filter then consumes exactly
/// as it would a real one.
///
/// Owns the scene index chain interposed into a single proxy shape's render
/// index:
///
///     emulation ─┐
///                ├─► merging ─► sceneGlobals ─► renderPass filter
///      retained ─┘
///
class MayaUsdRenderPassPublisher
{
public:
    /// Interposes a filter chain between \p renderIndex's emulation scene index
    /// and its merging scene index, and returns the publisher that drives it.
    ///
    /// HdRenderIndex only runs the scene index plugin registry's append
    /// callbacks when the render delegate has a non-empty display name
    /// (renderIndex.cpp:218-227), which is never true for the directly
    /// constructed HdVP2RenderDelegate. So instead of appending, this pulls the
    /// emulation input out of the render index's merging scene index and puts
    /// it back wrapped in the filter. Removing first is what makes the filter
    /// interpose rather than become a sibling that cannot see legacy prims.
    ///
    /// Must run before the scene delegate populates. Returns null (with a
    /// warning) if the expected scene index layout is not found.
    static std::unique_ptr<MayaUsdRenderPassPublisher> Attach(HdRenderIndex& renderIndex);

    /// Builds the chain over \p inputScene. \p terminal is the scene index to
    /// hand back to Hydra.
    MayaUsdRenderPassPublisher(const HdSceneIndexBaseRefPtr& inputScene);

    HdSceneIndexBaseRefPtr Terminal() const { return _terminal; }

    /// Reads the "prune" and "renderVisibility" collections from the render
    /// pass at \p passPath on \p stage and publishes them for the filter.
    ///
    /// \p delegateId is the UsdImagingDelegate's prefix; collection expressions
    /// are authored against raw USD paths, but the emulated scene sees them
    /// prefixed, so the expressions are rebased onto it.
    ///
    /// An empty \p passPath, a missing prim, or a prim carrying neither
    /// collection all deactivate filtering.
    void Publish(const UsdStageRefPtr& stage, const SdfPath& passPath, const SdfPath& delegateId);

private:
    HdRetainedSceneIndexRefPtr          _retained;
    HdsiSceneGlobalsSceneIndexRefPtr    _sceneGlobals;
    MayaUsdRenderPassSceneIndexRefPtr   _filter;
    HdSceneIndexBaseRefPtr              _terminal;

    // Path of the synthetic pass prim currently in _retained, if any.
    SdfPath _publishedPassPath;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAUSD_VP2_RENDER_PASS_PUBLISHER_H
