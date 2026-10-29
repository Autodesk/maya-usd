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
#include "renderDelegate.h"

#include <pxr/imaging/hd/enums.h>
#include <pxr/imaging/hd/rprim.h>
#include <pxr/imaging/hd/sceneDelegate.h>

#include <maya/MHWGeometry.h>

#include <memory>

namespace MAYAUSD_NS_DEF {

/*! \brief  HdVP2GaussianSplat-specific data shared among all its draw items.
    \class  HdVP2GaussianSplatSharedData
*/
struct HdVP2GaussianSplatSharedData
{
    //! Splat centers in local space.
    PXR_NS::VtVec3fArray _points;

    //! Position buffer of the Rprim to be shared among all its draw items.
    std::unique_ptr<MHWRender::MVertexBuffer> _positionsBuffer;

    //! Render item color buffer - use when updating data
    std::unique_ptr<MHWRender::MVertexBuffer> _colorBuffer;

    //! Render tag of the Rprim.
    PXR_NS::TfToken _renderTag;
};

/*! \brief  VP2 representation of Hydra particleField (3D Gaussian splat) prims.
    \class  HdVP2GaussianSplat

    Splats are drawn as opaque fat points, colored from the first spherical harmonics
    coefficient. Not true Gaussian splatting.
*/
class HdVP2GaussianSplat final
    : public PXR_NS::HdRprim
    , public PXR_NS::MayaUsdRPrim
{
public:
    HdVP2GaussianSplat(
        PXR_NS::HdVP2RenderDelegate* delegate,
        const PXR_NS::SdfPath&       id
#if defined(HD_API_VERSION) && HD_API_VERSION >= 36
    );
#else
        ,
        const PXR_NS::SdfPath& instancerId = PXR_NS::SdfPath());
#endif

    //! Destructor.
    ~HdVP2GaussianSplat() override = default;

    void Sync(
        PXR_NS::HdSceneDelegate* delegate,
        PXR_NS::HdRenderParam*   renderParam,
        PXR_NS::HdDirtyBits*     dirtyBits,
        const PXR_NS::TfToken&   reprToken) override;

    PXR_NS::HdDirtyBits GetInitialDirtyBitsMask() const override;

    const PXR_NS::TfTokenVector& GetBuiltinPrimvarNames() const override;

protected:
    PXR_NS::HdDirtyBits _PropagateDirtyBits(PXR_NS::HdDirtyBits bits) const override;

    void _InitRepr(const PXR_NS::TfToken& reprToken, PXR_NS::HdDirtyBits* dirtyBits) override;

    PXR_NS::TfToken& _RenderTag() override { return _splatSharedData._renderTag; }

private:
    void _UpdateRepr(PXR_NS::HdSceneDelegate* sceneDelegate, const PXR_NS::TfToken& reprToken);

    void _UpdateDrawItem(PXR_NS::HdSceneDelegate* sceneDelegate, PXR_NS::HdVP2DrawItem* drawItem);

    void _UpdateColorBuffer(PXR_NS::HdSceneDelegate* sceneDelegate);

    MHWRender::MRenderItem* _CreateSplatRenderItem(const MString& name) const;

    enum DirtyBits : PXR_NS::HdDirtyBits
    {
        DirtySelectionHighlight = PXR_NS::MayaUsdRPrim::DirtySelectionHighlight
    };

    //! Shared data for all draw items of the Rprim
    HdVP2GaussianSplatSharedData _splatSharedData;
};

} // namespace MAYAUSD_NS_DEF

#endif // PXR_VERSION >= 2605

#endif // HDVP2_GAUSSIAN_SPLAT_H
