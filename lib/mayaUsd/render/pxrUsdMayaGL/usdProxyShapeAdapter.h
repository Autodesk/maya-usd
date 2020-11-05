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
#ifndef PXRUSDMAYAGL_USD_PROXY_SHAPE_ADAPTER_H
#define PXRUSDMAYAGL_USD_PROXY_SHAPE_ADAPTER_H

/// \file pxrUsdMayaGL/usdProxyShapeAdapter.h

#include <memory>

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
#include <mayaUsd/render/pxrUsdMayaGL/shapeAdapter.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <maya/M3dView.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MPxSurfaceShape.h>

PXR_NAMESPACE_OPEN_SCOPE

class UsdMayaProxyDrawOverride;
class UsdMayaProxyShapeUI;

/// Class to manage translation of USD proxy shape node data and viewport state
/// for imaging with Hydra.
class PxrMayaHdUsdProxyShapeAdapter : public PxrMayaHdShapeAdapter
{
public:
    /// Update the shape adapter's visibility state from the display status
    /// of its shape.
    ///
    /// When a Maya shape is made invisible, it may no longer be included
    /// in the "prepare" phase of a viewport render (i.e. we get no
    /// getDrawRequests() or prepareForDraw() callbacks for that shape).
    /// This method can be called on demand to ensure that the shape
    /// adapter is updated with the current visibility state of the shape.
    ///
    /// The optional \p view parameter can be passed to have view-based
    /// state such as view and/or plugin object filtering factor into the
    /// shape's visibility.
    ///
    /// Returns true if the visibility state was changed, or false
    /// otherwise.
    MAYAUSD_CORE_PUBLIC
    bool UpdateVisibility(const M3dView* view = nullptr) override;

    /// Gets whether the shape adapter's shape is visible.
    ///
    /// This should be called after a call to UpdateVisibility() to ensure
    /// that the returned value is correct.
    MAYAUSD_CORE_PUBLIC
    bool IsVisible() const override;

    MAYAUSD_CORE_PUBLIC
    void SetRootXform(const GfMatrix4d& transform) override;

protected:
    /// Update the shape adapter's state from the shape with the given
    /// \p shapeDagPath and display state.
    ///
    /// This method should be called by both public versions of Sync() and
    /// should perform shape data updates that are common to both the
    /// legacy viewport and Viewport 2.0. The legacy viewport Sync() method
    /// "promotes" the display state parameters to their Viewport 2.0
    /// equivalents before calling this method.
    MAYAUSD_CORE_PUBLIC
    bool _Sync(
        const MDagPath&                shapeDagPath,
        const unsigned int             displayStyle,
        const MHWRender::DisplayStatus displayStatus) override;

    /// Construct a new uninitialized PxrMayaHdUsdProxyShapeAdapter.
    ///
    /// Note that only friends of this class are able to construct
    /// instances of this class.
    MAYAUSD_CORE_PUBLIC
    PxrMayaHdUsdProxyShapeAdapter(bool isViewport2);

    MAYAUSD_CORE_PUBLIC
    ~PxrMayaHdUsdProxyShapeAdapter() override;

private:
    /// Initialize the shape adapter using the given \p renderIndex.
    ///
    /// This method is called automatically during Sync() when the shape
    /// adapter's "identity" changes. This can happen when the shape
    /// managed by this adapter is changed by setting a new DAG path, or
    /// otherwise when there is some other fundamental change to the shape
    /// or to the delegate or render index.
    /// The shape adapter will then query the batch renderer for its render
    /// index and use that to re-create its delegate and re-add its rprim
    /// collection, if necessary.
    MAYAUSD_CORE_PUBLIC
    bool _Init(HdRenderIndex* renderIndex);

    UsdPrim       _rootPrim;
    SdfPathVector _excludedPrimPaths;

    std::shared_ptr<UsdImagingDelegate> _delegate;

    /// The classes that maintain ownership of and are responsible for
    /// updating the shape adapter for their shape are made friends of
    /// PxrMayaHdUsdProxyShapeAdapter so that they alone can set properties
    /// on the shape adapter.
    friend class UsdMayaProxyDrawOverride;
    friend class UsdMayaProxyShapeUI;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
