//
// Copyright 2022 Autodesk
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
#ifndef HDVP2_POINTS_H
#define HDVP2_POINTS_H

#include "mayaPrimCommon.h"

#include <mayaUsd/render/vp2RenderDelegate/proxyRenderDelegate.h>

#include <pxr/imaging/hd/points.h>
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

/*! \brief  HdVP2Points-specific data shared among all its draw items.
    \class  HdVP2PointsSharedData

    A Rprim can have multiple draw items. The shared data are extracted from
    USD scene delegate during synchronization. Then each draw item can prepare
    draw data from these shared data as needed.
*/
struct HdVP2PointsSharedData
{
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

/*! \brief  VP2 representation of Hydra points.
    \class  HdVP2Points
*/
class HdVP2Points final
    : public HdPoints
    , public MayaUsdRPrim
{
public:
    HdVP2Points(
        HdVP2RenderDelegate* delegate, const SdfPath& id
#if defined(HD_API_VERSION) && HD_API_VERSION >= 36
        );
#else
        , const SdfPath& instancerId = SdfPath());
#endif

    //! Destructor.
    ~HdVP2Points() override = default;

    void Sync(
        HdSceneDelegate* delegate,
        HdRenderParam*   renderParam,
        HdDirtyBits*     dirtyBits,
        TfToken const&   reprToken) override;

    HdDirtyBits GetInitialDirtyBitsMask() const override;

protected:
    HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

    void _InitRepr(TfToken const& reprToken, HdDirtyBits* dirtyBits) override;

    TfToken& _RenderTag() override { return _pointsSharedData._renderTag; }

private:
    void _UpdateRepr(HdSceneDelegate* sceneDelegate, TfToken const& reprToken);

    void _UpdateDrawItem(
        HdSceneDelegate*             sceneDelegate,
        HdVP2DrawItem*               drawItem,
        HdPointsReprDesc const& desc);

    void _UpdatePrimvarSources(
        HdSceneDelegate*     sceneDelegate,
        HdDirtyBits          dirtyBits,
        TfTokenVector const& requiredPrimvars);

    MHWRender::MRenderItem* _CreateFatPointsRenderItem(const MString& name) const;

    enum DirtyBits : HdDirtyBits
    {
        DirtySelectionHighlight = MayaUsdRPrim::DirtySelectionHighlight
    };

    //! Shared data for all draw items of the Rprim
    HdVP2PointsSharedData _pointsSharedData;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDVP2_POINTS_H
