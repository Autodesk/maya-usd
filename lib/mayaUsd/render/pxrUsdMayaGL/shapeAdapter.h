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
#ifndef PXRUSDMAYAGL_SHAPE_ADAPTER_H
#define PXRUSDMAYAGL_SHAPE_ADAPTER_H

/// \file pxrUsdMayaGL/shapeAdapter.h

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/token.h>
#include <pxr/imaging/hd/changeTracker.h>
#include <pxr/imaging/hd/repr.h>
#include <pxr/imaging/hd/rprimCollection.h>
#include <pxr/imaging/hd/types.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>

#include <memory>
#include <unordered_map>

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
#include <pxr/usd/sdf/types.h>

#include <maya/M3dView.h>
#undef Always // Defined in /usr/lib/X11/X.h (eventually included by M3dView.h) - breaks
              // pxr/usd/lib/usdUtils/registeredVariantSet.h
#include <mayaUsd/base/api.h>
#include <mayaUsd/render/pxrUsdMayaGL/renderParams.h>
#include <mayaUsd/render/pxrUsdMayaGL/userData.h>

#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MDrawRequest.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MPxSurfaceShapeUI.h>
#include <maya/MUserData.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Abstract base class for objects that manage translation of Maya shape node
/// data and viewport state for imaging with Hydra.
class PxrMayaHdShapeAdapter
{
public:
    /// Update the shape adapter's state from the shape with the given
    /// \p shapeDagPath and the legacy viewport display state.
    MAYAUSD_CORE_PUBLIC
    bool Sync(
        const MDagPath&              shapeDagPath,
        const M3dView::DisplayStyle  legacyDisplayStyle,
        const M3dView::DisplayStatus legacyDisplayStatus);

    /// Update the shape adapter's state from the shape with the given
    /// \p shapeDagPath and the Viewport 2.0 display state.
    MAYAUSD_CORE_PUBLIC
    bool Sync(
        const MDagPath&                shapeDagPath,
        const unsigned int             displayStyle,
        const MHWRender::DisplayStatus displayStatus);

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
    virtual bool UpdateVisibility(const M3dView* view = nullptr);

    /// Gets whether the shape adapter's shape is visible.
    ///
    /// This should be called after a call to UpdateVisibility() to ensure
    /// that the returned value is correct.
    MAYAUSD_CORE_PUBLIC
    virtual bool IsVisible() const;

    /// Get the Maya user data object for drawing in the legacy viewport.
    ///
    /// This Maya user data is attached to the given \p drawRequest. Its
    /// lifetime is *not* managed by Maya, so the batch renderer deletes it
    /// manually at the end of a legacy viewport Draw().
    ///
    /// \p boundingBox may be set to nullptr if no box is desired to be
    /// drawn.
    ///
    MAYAUSD_CORE_PUBLIC
    virtual void GetMayaUserData(
        MPxSurfaceShapeUI*  shapeUI,
        MDrawRequest&       drawRequest,
        const MBoundingBox* boundingBox = nullptr);

    /// Get the Maya user data object for drawing in Viewport 2.0.
    ///
    /// \p oldData should be the same \p oldData parameter that Maya passed
    /// into the calling prepareForDraw() method. The return value from
    /// this method should then be returned back to Maya in the calling
    /// prepareForDraw().
    ///
    /// Note that this version of GetMayaUserData() is also invoked by the
    /// legacy viewport version, in which case we expect oldData to be
    /// nullptr.
    ///
    /// \p boundingBox may be set to nullptr if no box is desired to be
    /// drawn.
    ///
    /// Returns a pointer to a new PxrMayaHdUserData object populated with
    /// the given parameters if oldData is nullptr (or not an instance of
    /// PxrMayaHdUserData), otherwise returns oldData after having
    /// re-populated it.
    MAYAUSD_CORE_PUBLIC
    virtual PxrMayaHdUserData*
    GetMayaUserData(MUserData* oldData, const MBoundingBox* boundingBox = nullptr);

    /// Gets the HdReprSelector that corresponds to the given Maya display
    /// style.
    ///
    /// \p displayStyle should be a bitwise combination of
    /// MHWRender::MFrameContext::DisplayStyle values, typically either
    /// up-converted from a single M3dView::DisplayStyle value obtained
    /// using MDrawInfo::displayStyle() for the legacy viewport, or
    /// obtained using MHWRender::MFrameContext::getDisplayStyle() for
    /// Viewport 2.0.
    ///
    /// The HdReprSelector chosen is also dependent on the display status
    /// (active/selected vs. inactive) which Maya is queried for, as well
    /// as whether or not the render params specify that we are using the
    /// shape's wireframe, which is influenced by both the display status
    /// and whether or not the shape is involved in a soft selection.
    ///
    /// If there is no corresponding HdReprSelector for the given display
    /// style, an empty HdReprSelector is returned.
    MAYAUSD_CORE_PUBLIC
    HdReprSelector GetReprSelectorForDisplayStyle(unsigned int displayStyle) const;

    /// Get the render params for the shape adapter's current state.
    const PxrMayaHdRenderParams& GetRenderParams() const { return _renderParams; }

    /// Get the rprim collection for the given repr.
    ///
    /// These collections are created when the shape adapter's MDagPath is
    /// set to a valid Maya shape.
    ///
    /// Returns an empty collection if there is no collection for the given
    /// repr.
    const HdRprimCollection& GetRprimCollection(const HdReprSelector& repr) const
    {
        const auto& iter = _rprimCollectionMap.find(repr);
        if (iter != _rprimCollectionMap.cend()) {
            return iter->second;
        }

        static const HdRprimCollection emptyCollection;
        return emptyCollection;
    }

    /// Get the ID of the render task for the collection of the given repr.
    ///
    /// These render task IDs are created when the shape adapter's MDagPath
    /// is set to a valid Maya shape.
    ///
    /// Returns an empty SdfPath if there is no render task ID for the
    /// given repr.
    const SdfPath& GetRenderTaskId(const HdReprSelector& repr) const
    {
        const auto& iter = _renderTaskIdMap.find(repr);
        if (iter != _renderTaskIdMap.cend()) {
            return iter->second;
        }

        static const SdfPath emptyTaskId;
        return emptyTaskId;
    }

    /// Retrieves the render tags for this shape (i.e. which prim purposes
    /// should be drawn, such as geometry, proxy, guide and/or render).
    ///
    /// This function just returns the _renderTags attribute and it is
    /// expected that subclasses update the attribute in _Sync().
    const TfTokenVector& GetRenderTags() const { return _renderTags; }

    const GfMatrix4d& GetRootXform() const { return _rootXform; }

    /// Sets the root transform for the shape adapter.
    ///
    /// This function is virtual in case the shape adapter needs to update
    /// other state in response to a change in the root transform (e.g.
    /// updating an HdSceneDelegate).
    virtual void SetRootXform(const GfMatrix4d& transform) { _rootXform = transform; }

    const SdfPath& GetDelegateID() const { return _delegateId; }

    const MDagPath& GetDagPath() const { return _shapeDagPath; }

    /// Get whether this shape adapter is for use with Viewport 2.0.
    ///
    /// The shape adapter's viewport renderer affiliation must be specified
    /// when it is constructed.
    ///
    /// Returns true if the shape adapter should be used for batched
    /// drawing/selection in Viewport 2.0, or false if it should be used
    /// in the legacy viewport.
    bool IsViewport2() const { return _isViewport2; }

protected:
    /// Update the shape adapter's state from the shape with the given
    /// \p dagPath and display state.
    ///
    /// This method should be called by both public versions of Sync() and
    /// should perform shape data updates that are common to both the
    /// legacy viewport and Viewport 2.0. The legacy viewport Sync() method
    /// "promotes" the display state parameters to their Viewport 2.0
    /// equivalents before calling this method.
    MAYAUSD_CORE_PUBLIC
    virtual bool _Sync(
        const MDagPath&                shapeDagPath,
        const unsigned int             displayStyle,
        const MHWRender::DisplayStatus displayStatus)
        = 0;

    /// Sets the shape adapter's DAG path.
    ///
    /// This re-computes the "identifier" for the shape, which is used to
    /// compute the names of the shape's HdRprimCollections. The batch
    /// renderer will create a render task for each of the shape's
    /// HdRprimCollections, and those render tasks are identified by an
    /// SdfPath constructed using the collection's name. We therefore need
    /// the collections to have names that are unique to the shape they
    /// represent and also sanitized for use in SdfPaths.
    ///
    /// The identifier will be a TfToken that is unique to the shape and is
    /// a valid SdfPath identifier, or an empty TfToken if there is an
    /// error, which can be detected from the returned MStatus.
    MAYAUSD_CORE_PUBLIC
    MStatus _SetDagPath(const MDagPath& dagPath);

    /// Mark the render tasks for this shape dirty.
    ///
    /// The batch renderer currently creates a render task for each shape's
    /// HdRprimCollection, but it has no knowledge of when the collection
    /// is changed, for example. This method should be called to notify the
    /// batch renderer when such a change is made.
    MAYAUSD_CORE_PUBLIC
    void _MarkRenderTasksDirty(HdDirtyBits dirtyBits = HdChangeTracker::AllDirty);

    /// Helper for getting the wireframe color of the shape.
    ///
    /// Determining the wireframe color may involve inspecting the soft
    /// selection, for which the batch renderer manages a helper. This
    /// class is made a friend of the batch renderer class so that it can
    /// access the soft selection info.
    ///
    /// Returns true if the wireframe color should be used, that is if the
    /// object and/or its component(s) are involved in a selection.
    /// Otherwise returns false.
    ///
    /// Note that we do not factor in the viewport's displayStyle, which
    /// may indicate that a wireframe style is being drawn (either
    /// kWireFrame or kBoundingBox). The displayStyle can be changed
    /// without triggering a re-Sync(), so we want to make sure that shape
    /// adapters don't inadvertently "bake in" whether to use the wireframe
    /// into their render params based on it. We only want to know whether
    /// we need to use the wireframe for a reason *other* than the
    /// displayStyle.
    ///
    /// The wireframe color will always be returned in \p wireframeColor
    /// (if it is not nullptr) in case the caller wants to use other
    /// criteria for determining whether to use it (e.g. for bounding
    /// boxes).
    static bool _GetWireframeColor(
        MHWRender::DisplayStatus displayStatus,
        const MDagPath&          shapeDagPath,
        GfVec4f*                 wireframeColor);

    /// Helper for computing the viewport visibility of the shape.
    ///
    /// Takes into account the visibility attribute on the shape and its
    /// DAG ancestors. If an M3dView is provided to \p view, then
    /// view-based state such as view and/or plugin object filtering will
    /// also be factored into the shape's visibility.
    ///
    /// Returns true if computing the visibility was successful, false if
    /// there was an error. The visibility is returned in \p visibility.
    static bool _GetVisibility(const MDagPath& dagPath, const M3dView* view, bool* visibility);

    /// Construct a new uninitialized PxrMayaHdShapeAdapter.
    MAYAUSD_CORE_PUBLIC
    PxrMayaHdShapeAdapter(bool isViewport2);

    MAYAUSD_CORE_PUBLIC
    virtual ~PxrMayaHdShapeAdapter();

    TfToken _shapeIdentifier;
    SdfPath _delegateId;

    PxrMayaHdRenderParams _renderParams;

    struct _ReprHashFunctor
    {
        size_t operator()(const HdReprSelector& repr) const
        {
            // Since we currently only use the refinedToken of
            // HdReprSelector, we only need to consider that one token when
            // hashing.
            return repr[0u].Hash();
        }
    };

    // Mapping of HdReprSelector to the rprim collection for that selector.
    using _RprimCollectionMap
        = std::unordered_map<const HdReprSelector, const HdRprimCollection, _ReprHashFunctor>;
    _RprimCollectionMap _rprimCollectionMap;

    // Mapping of HdReprSelector to the ID of the render task for the
    // collection for that selector.
    using _RenderTaskIdMap
        = std::unordered_map<const HdReprSelector, const SdfPath, _ReprHashFunctor>;
    _RenderTaskIdMap _renderTaskIdMap;

    TfTokenVector _renderTags;

    GfMatrix4d _rootXform;

    const bool _isViewport2;

private:
    MDagPath _shapeDagPath;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
