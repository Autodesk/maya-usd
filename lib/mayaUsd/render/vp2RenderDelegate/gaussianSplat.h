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

#include "gaussianSplatInstancing.h"
#include "gaussianSplatItems.h"
#include "gaussianSplatResources.h"
#include "gaussianSplatSH.h"
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

    //! SH0 color stream (float4, rgb and 1). The packed float4 streams use the kTexture
    //! semantic, because an instanced draw binding the kColor semantic with another float4
    //! stream draws nothing.
    std::unique_ptr<MHWRender::MVertexBuffer> _colorBuffer;

    //! Orientation stream (float4, quaternion x y z w).
    std::unique_ptr<MHWRender::MVertexBuffer> _orientationBuffer;

    //! Scale and opacity stream (float4, scale x y z and opacity).
    std::unique_ptr<MHWRender::MVertexBuffer> _scaleOpacityBuffer;

    //! Render tag of the Rprim.
    PXR_NS::TfToken _renderTag;
};

/*! \brief  VP2 representation of Hydra particleField (3D Gaussian splat) prims.
    \class  HdVP2GaussianSplat

    Splats are drawn as opaque oriented and scaled quads expanded by a geometry stage shader,
    colored from their spherical harmonics. Opacity is stochastic: each fragment is kept with a
    probability equal to the splat opacity, so there is no sorting and no blending.

    The Rprim reads the splat data and fills the vertex streams (gaussianSplat.cpp), packs the
    spherical harmonics (gaussianSplatSH), owns its shader clones and SH texture
    (gaussianSplatResources), creates one item per repr token (gaussianSplatItems) and commits
    its instances (gaussianSplatInstancing).
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

    void _UpdateDrawItem(
        PXR_NS::HdSceneDelegate* sceneDelegate,
        PXR_NS::HdVP2DrawItem*   drawItem,
        const PXR_NS::TfToken&   reprToken);

    HdVP2GaussianSplatInstancing _UpdateInstancing(
        PXR_NS::HdSceneDelegate*    sceneDelegate,
        PXR_NS::HdVP2DrawItem*      drawItem,
        const PXR_NS::TfToken&      reprToken,
        bool                        isWire,
        PXR_NS::MayaUsdCommitState& stateToCommit);

    void _UpdateColorBuffer(const HdVP2GaussianSplatSHCoefficients& sh);

    void _UpdateOrientationScaleOpacityBuffers(PXR_NS::HdSceneDelegate* sceneDelegate);

    MHWRender::MRenderItem*
    _CreateSplatRenderItem(const MString& name, const HdVP2GaussianSplatItemDesc& desc) const;

    enum DirtyBits : PXR_NS::HdDirtyBits
    {
        DirtySelectionHighlight = PXR_NS::MayaUsdRPrim::DirtySelectionHighlight
    };

    //! Shared data for all draw items of the Rprim
    HdVP2GaussianSplatSharedData _splatSharedData;

    //! Shader clones and SH texture of the Rprim. Declared after the shared data, so they are
    //! released before the vertex buffers.
    HdVP2GaussianSplatResources _resources;
};

} // namespace MAYAUSD_NS_DEF

#endif // PXR_VERSION >= 2605

#endif // HDVP2_GAUSSIAN_SPLAT_H
