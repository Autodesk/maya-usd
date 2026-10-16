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
#ifndef HDVP2_GAUSSIAN_SPLAT_H
#define HDVP2_GAUSSIAN_SPLAT_H

#include <pxr/pxr.h>

#if PXR_VERSION >= 2605

#include "mayaPrimCommon.h"

#include <pxr/imaging/hd/enums.h>
#include <pxr/imaging/hd/rprim.h>

#include <maya/MHWGeometry.h>

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
class HdVP2DrawItem;
class HdVP2RenderDelegate;

/*! \brief  HdVP2GaussianSplat-specific data shared among all its draw items.
    \class  HdVP2GaussianSplatSharedData
*/
struct HdVP2GaussianSplatSharedData
{
    //! Splat centers in local space.
    VtVec3fArray _points;

    //! Per-splat opacities, used to skip splats that would not be visible.
    VtFloatArray _opacities;

    //! Number of splats with a visible opacity, i.e. the size of the index buffer.
    unsigned int _numVisibleSplats { 0 };

    //! Position buffer of the Rprim to be shared among all its draw items.
    std::unique_ptr<MHWRender::MVertexBuffer> _positionsBuffer;

    //! Render item color buffer - use when updating data
    std::unique_ptr<MHWRender::MVertexBuffer> _colorBuffer;

    //! Render tag of the Rprim.
    TfToken _renderTag;
};

/*! \brief  VP2 representation of Hydra particleField (3D Gaussian splat) prims.
    \class  HdVP2GaussianSplat

    Splats are drawn as opaque fat points, colored from the first spherical harmonics
    coefficient. Not true Gaussian splatting.
*/
class HdVP2GaussianSplat final
    : public HdRprim
    , public MayaUsdRPrim
{
public:
    HdVP2GaussianSplat(
        HdVP2RenderDelegate* delegate,
        const SdfPath&       id
#if defined(HD_API_VERSION) && HD_API_VERSION >= 36
    );
#else
        ,
        const SdfPath& instancerId = SdfPath());
#endif

    //! Destructor.
    ~HdVP2GaussianSplat() override = default;

    void Sync(
        HdSceneDelegate* delegate,
        HdRenderParam*   renderParam,
        HdDirtyBits*     dirtyBits,
        const TfToken&   reprToken) override;

    HdDirtyBits GetInitialDirtyBitsMask() const override;

    const TfTokenVector& GetBuiltinPrimvarNames() const override;

protected:
    HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

    void _InitRepr(const TfToken& reprToken, HdDirtyBits* dirtyBits) override;

    TfToken& _RenderTag() override { return _splatSharedData._renderTag; }

private:
    void _UpdateRepr(HdSceneDelegate* sceneDelegate, const TfToken& reprToken);

    void _UpdateDrawItem(HdSceneDelegate* sceneDelegate, HdVP2DrawItem* drawItem);

    void _UpdateColorBuffer(HdSceneDelegate* sceneDelegate);

    void _UpdateIndexBuffer(
        HdVP2DrawItem::RenderItemData& drawItemData,
        MayaUsdCommitState&            stateToCommit) const;

    MHWRender::MRenderItem* _CreateSplatRenderItem(const MString& name) const;

    enum DirtyBits : HdDirtyBits
    {
        DirtySelectionHighlight = MayaUsdRPrim::DirtySelectionHighlight
    };

    //! Shared data for all draw items of the Rprim
    HdVP2GaussianSplatSharedData _splatSharedData;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2605

#endif // HDVP2_GAUSSIAN_SPLAT_H
