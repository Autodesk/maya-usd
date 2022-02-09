//
// Copyright 2020 Autodesk
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
#ifndef HDVP2_BASIS_CURVES_H
#define HDVP2_BASIS_CURVES_H

#include "mayaPrimCommon.h"

#include <mayaUsd/render/vp2RenderDelegate/proxyRenderDelegate.h>

#include <pxr/base/vt/array.h>
#include <pxr/imaging/hd/basisCurves.h>
#include <pxr/imaging/hd/enums.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MHWGeometry.h>

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
class HdVP2DrawItem;
class HdVP2RenderDelegate;

//! A primvar vertex buffer map indexed by primvar name.
using PrimvarBufferMap
    = std::unordered_map<TfToken, std::unique_ptr<MHWRender::MVertexBuffer>, TfToken::HashFunctor>;

/*! \brief  HdVP2BasisCurves-specific data shared among all its draw items.
    \class  HdVP2BasisCurvesSharedData

    A Rprim can have multiple draw items. The shared data are extracted from
    USD scene delegate during synchronization. Then each draw item can prepare
    draw data from these shared data as needed.
*/
struct HdVP2BasisCurvesSharedData
{
    //! Cached scene data. VtArrays are reference counted, so as long as we
    //! only call const accessors keeping them around doesn't incur a buffer
    //! copy.
    HdBasisCurvesTopology _topology;

    //! A local cache of primvar scene data. "data" is a copy-on-write handle to
    //! the actual primvar buffer, and "interpolation" is the interpolation mode
    //! to be used.
    struct PrimvarSource
    {
        VtValue         data;
        HdInterpolation interpolation;
    };
    TfHashMap<TfToken, PrimvarSource, TfToken::HashFunctor> _primvarSourceMap;

    //! Render item primvar buffers - use when updating data
    PrimvarBufferMap _primvarBuffers;

    //! A local cache of points. It is not cached in the above primvar map
    //! but a separate VtArray for easier access.
    VtVec3fArray _points;

    //! Position buffer of the Rprim to be shared among all its draw items.
    std::unique_ptr<MHWRender::MVertexBuffer> _positionsBuffer;

    //! Render item color buffer - use when updating data
    std::unique_ptr<MHWRender::MVertexBuffer> _colorBuffer;

    //! Render item normals buffer - use when updating data
    std::unique_ptr<MHWRender::MVertexBuffer> _normalsBuffer;

    //! The display style.
    HdDisplayStyle _displayStyle;

    //! Render tag of the Rprim.
    TfToken _renderTag;
};

/*! \brief  VP2 representation of basis curves.
    \class  HdVP2BasisCurves

    The prim object's main function is to bridge the scene description and the
    renderable representation. The Hydra image generation algorithm will call
    HdRenderIndex::SyncAll() before any drawing; this, in turn, will call
    Sync() for each Rprim with new data.

    Sync() is passed a set of dirtyBits, indicating which scene buffers are
    dirty. It uses these to pull all of the new scene data and constructs
    updated geometry objects.  Commit of changed buffers to GPU happens
    in HdVP2RenderDelegate::CommitResources(), which runs on main-thread after
    all Rprims have been updated.
*/
class HdVP2BasisCurves final
    : public HdBasisCurves
    , public MayaUsdRPrim
{
public:
    HdVP2BasisCurves(
        HdVP2RenderDelegate* delegate,
#if defined(HD_API_VERSION) && HD_API_VERSION >= 36
        const SdfPath& id);
#else
        const SdfPath& id,
        const SdfPath& instancerId = SdfPath());
#endif

    //! Destructor.
    ~HdVP2BasisCurves() override = default;

    void Sync(
        HdSceneDelegate* delegate,
        HdRenderParam*   renderParam,
        HdDirtyBits*     dirtyBits,
        TfToken const&   reprToken) override;

    HdDirtyBits GetInitialDirtyBitsMask() const override;

protected:
    HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

    void _InitRepr(TfToken const& reprToken, HdDirtyBits* dirtyBits) override;

private:
    void _UpdateRepr(HdSceneDelegate* sceneDelegate, TfToken const& reprToken);

    void _UpdateDrawItem(
        HdSceneDelegate*             sceneDelegate,
        HdVP2DrawItem*               drawItem,
        HdBasisCurvesReprDesc const& desc);

    void _HideAllDrawItems(const TfToken& reprToken);

    void _UpdatePrimvarSources(
        HdSceneDelegate*     sceneDelegate,
        HdDirtyBits          dirtyBits,
        TfTokenVector const& requiredPrimvars);

    MHWRender::MRenderItem* _CreatePatchRenderItem(const MString& name) const;

    enum DirtyBits : HdDirtyBits
    {
        DirtySelectionHighlight = MayaUsdRPrim::DirtySelectionHighlight
    };

    //! Shared data for all draw items of the Rprim
    HdVP2BasisCurvesSharedData _curvesSharedData;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDVP2_BASIS_CURVES_H
