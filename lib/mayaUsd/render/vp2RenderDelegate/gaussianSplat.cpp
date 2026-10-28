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
#include "gaussianSplat.h"

#if PXR_VERSION >= 2605

#include "drawItem.h"
#include "renderDelegate.h"

#include <pxr/base/arch/hints.h>
#include <pxr/base/gf/half.h>
#include <pxr/base/gf/math.h>
#include <pxr/base/gf/vec3h.h>
#include <pxr/base/trace/trace.h>
#include <pxr/base/vt/types.h>
#include <pxr/base/vt/value.h>
#include <pxr/imaging/hd/repr.h>
#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/usd/usdVol/tokens.h>

#include <maya/MProfiler.h>
#include <maya/MSelectionMask.h>

namespace MAYAUSD_NS_DEF {

namespace {

//! Factor converting the first SH coefficient to a color: rgb = 0.5 + kSH_C0 * coeff.
constexpr float kSH_C0 = 0.28209479177387814f;

//! Highlight colors with alpha below this are drawn as transparent.
constexpr float kOpaqueAlphaThreshold = 0.999f;

//! Positions and colors can be authored in float or half precision.
bool _GetVec3fArray(const PXR_NS::VtValue& value, PXR_NS::VtVec3fArray& result)
{
    const PXR_NS::VtValue cast = PXR_NS::VtValue::Cast<PXR_NS::VtVec3fArray>(value);
    if (cast.IsEmpty()) {
        return false;
    }
    result = cast.UncheckedGet<PXR_NS::VtVec3fArray>();
    return true;
}

} // anonymous namespace

//! \brief  Constructor
HdVP2GaussianSplat::HdVP2GaussianSplat(
    PXR_NS::HdVP2RenderDelegate* delegate,
    const PXR_NS::SdfPath&       id
#if defined(HD_API_VERSION) && HD_API_VERSION >= 36
    )
    : PXR_NS::HdRprim(id)
#else
    ,
    const PXR_NS::SdfPath& instancerId)
    : PXR_NS::HdRprim(id, instancerId)
#endif
    , PXR_NS::MayaUsdRPrim(delegate, id)
{
    const MHWRender::MVertexBufferDescriptor desc(
        "", MHWRender::MGeometry::kPosition, MHWRender::MGeometry::kFloat, 3);

    _splatSharedData._positionsBuffer.reset(new MHWRender::MVertexBuffer(desc));
}

//! \brief  Synchronize VP2 state with scene delegate state based on dirty bits and repr
void HdVP2GaussianSplat::Sync(
    PXR_NS::HdSceneDelegate* delegate,
    PXR_NS::HdRenderParam*   renderParam,
    PXR_NS::HdDirtyBits*     dirtyBits,
    const PXR_NS::TfToken&   reprToken)
{
    if (!_SyncCommon(*this, delegate, renderParam, dirtyBits, _GetRepr(reprToken), reprToken)) {
        return;
    }

    MProfilingScope profilingScope(
        PXR_NS::HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorC_L2,
        _rprimId.asChar(),
        "HdVP2GaussianSplat::Sync");

    // All splat attributes (positions, SH coefficients) are primvars.
    if (*dirtyBits & PXR_NS::HdChangeTracker::DirtyPrimvar) {
        const PXR_NS::SdfPath& id = GetId();

        if (_GetVec3fArray(
                delegate->Get(id, PXR_NS::UsdVolTokens->positions), _splatSharedData._points)) {
            _CommitPositionsBuffer(
                _splatSharedData._positionsBuffer.get(), _splatSharedData._points);
        } else {
            TF_WARN("Gaussian splat %s has no positions.", id.GetText());
            _splatSharedData._points.clear();
        }

        _UpdateColorBuffer(delegate);
    }

    _SyncEndCommon(*this, delegate, dirtyBits, reprToken, _sharedData, _reprs);

    // Draw item update is controlled by its own dirty bits.
    _UpdateRepr(delegate, reprToken);
}

/*! \brief  Fill the color buffer from the first spherical harmonics coefficient of each splat.

    Splats are drawn opaque, so alpha is always 1.
*/
void HdVP2GaussianSplat::_UpdateColorBuffer(PXR_NS::HdSceneDelegate* sceneDelegate)
{
    const PXR_NS::SdfPath& id = GetId();
    const size_t           numSplats = _splatSharedData._points.size();
    if (numSplats == 0) {
        return;
    }

    // The coefficients are stored splat by splat, shStride coefficients per splat.
    PXR_NS::VtVec3fArray shCoeffs;
    _GetVec3fArray(
        sceneDelegate->Get(id, PXR_NS::UsdVolTokens->radianceSphericalHarmonicsCoefficients),
        shCoeffs);
    const size_t shStride = shCoeffs.size() / numSplats;
    const bool   hasSH = shStride > 0 && shCoeffs.size() == shStride * numSplats;

    if (!_splatSharedData._colorBuffer) {
        const MHWRender::MVertexBufferDescriptor vbDesc(
            "", MHWRender::MGeometry::kColor, MHWRender::MGeometry::kFloat, 4);

        _splatSharedData._colorBuffer.reset(new MHWRender::MVertexBuffer(vbDesc));
    }

    float* bufferData
        = static_cast<float*>(_splatSharedData._colorBuffer->acquire(numSplats, true));
    if (!bufferData) {
        return;
    }

    unsigned int offset = 0;
    if (hasSH) {
        for (size_t i = 0; i < numSplats; ++i) {
            // 3DGS band 0 (constant color) SH to RGB: rgb = 0.5 + C0 * coeff.
            const PXR_NS::GfVec3f color = PXR_NS::GfVec3f(0.5f) + kSH_C0 * shCoeffs[i * shStride];
            // SH can learn values that give out of range colors, clamp them.
            bufferData[offset++] = PXR_NS::GfClamp(color[0], 0.0f, 1.0f);
            bufferData[offset++] = PXR_NS::GfClamp(color[1], 0.0f, 1.0f);
            bufferData[offset++] = PXR_NS::GfClamp(color[2], 0.0f, 1.0f);
            bufferData[offset++] = 1.0f;
        }
    } else {
        // Without usable SH coefficients every splat gets the default color.
        TF_WARN(
            "Gaussian splat %s: missing or mismatched SH coefficients (%zu for %zu splats), "
            "using default color.",
            id.GetText(),
            shCoeffs.size(),
            numSplats);

        auto* const param = static_cast<PXR_NS::HdVP2RenderParam*>(_delegate->GetRenderParam());
        const PXR_NS::GfVec3f defaultColor
            = param->GetDrawScene().GetDefaultColor(PXR_NS::HdPrimTypeTokens->points);

        for (size_t i = 0; i < numSplats; ++i) {
            bufferData[offset++] = defaultColor[0];
            bufferData[offset++] = defaultColor[1];
            bufferData[offset++] = defaultColor[2];
            bufferData[offset++] = 1.0f;
        }
    }

    _CommitMVertexBuffer(_splatSharedData._colorBuffer.get(), bufferData);
}

/*! \brief  Returns the names of built-in primvars, i.e. primvars that are part of the core
            geometric schema for this prim.
*/
const PXR_NS::TfTokenVector& HdVP2GaussianSplat::GetBuiltinPrimvarNames() const
{
    static const PXR_NS::TfTokenVector primvarNames
        = { PXR_NS::UsdVolTokens->positions,
            PXR_NS::UsdVolTokens->opacities,
            PXR_NS::UsdVolTokens->radianceSphericalHarmonicsCoefficients };
    return primvarNames;
}

/*! \brief  Returns the minimal set of dirty bits to place in the
            change tracker for use in the first sync of this prim.
*/
PXR_NS::HdDirtyBits HdVP2GaussianSplat::GetInitialDirtyBitsMask() const
{
    constexpr PXR_NS::HdDirtyBits bits = PXR_NS::HdChangeTracker::InitRepr
        | PXR_NS::HdChangeTracker::DirtyExtent | PXR_NS::HdChangeTracker::DirtyPoints
        | PXR_NS::HdChangeTracker::DirtyPrimID | PXR_NS::HdChangeTracker::DirtyPrimvar
        | PXR_NS::HdChangeTracker::DirtyDisplayStyle | PXR_NS::HdChangeTracker::DirtyRepr
        | PXR_NS::HdChangeTracker::DirtyMaterialId | PXR_NS::HdChangeTracker::DirtyTransform
        | PXR_NS::HdChangeTracker::DirtyVisibility | PXR_NS::HdChangeTracker::DirtyRenderTag
        | DirtySelectionHighlight;

    return bits;
}

//! \brief  Add additional dirty bits
PXR_NS::HdDirtyBits HdVP2GaussianSplat::_PropagateDirtyBits(PXR_NS::HdDirtyBits bits) const
{
    _PropagateDirtyBitsCommon(bits, _reprs);
    return bits;
}

//! \brief  Initialize the given representation of this Rprim.
void HdVP2GaussianSplat::_InitRepr(const PXR_NS::TfToken& reprToken, PXR_NS::HdDirtyBits* dirtyBits)
{
    auto* const         param = static_cast<PXR_NS::HdVP2RenderParam*>(_delegate->GetRenderParam());
    MSubSceneContainer* subSceneContainer = param->GetContainer();
    if (ARCH_UNLIKELY(!subSceneContainer)) {
        return;
    }

    PXR_NS::HdReprSharedPtr repr = _InitReprCommon(*this, reprToken, _reprs, dirtyBits, GetId());
    if (!repr) {
        return;
    }

#if HD_API_VERSION < 35
    auto* drawItem = new PXR_NS::HdVP2DrawItem(_delegate, &_sharedData);
#else
    std::unique_ptr<PXR_NS::HdVP2DrawItem> drawItem
        = std::make_unique<PXR_NS::HdVP2DrawItem>(_delegate, &_sharedData);
#endif

    MHWRender::MRenderItem* renderItem = _CreateSplatRenderItem(drawItem->GetDrawItemName());
    drawItem->AddUsage(PXR_NS::HdVP2DrawItem::kSelectionHighlight);
    _AddRenderItem(*drawItem, renderItem, *subSceneContainer);

#if HD_API_VERSION < 35
    repr->AddDrawItem(drawItem);
#else
    repr->AddDrawItem(std::move(drawItem));
#endif
}

//! \brief  Create the fat points render item used to draw the splats.
MHWRender::MRenderItem* HdVP2GaussianSplat::_CreateSplatRenderItem(const MString& name) const
{
    MHWRender::MRenderItem* const renderItem = MHWRender::MRenderItem::Create(
        name, MHWRender::MRenderItem::MaterialSceneItem, MHWRender::MGeometry::kPoints);

    renderItem->setDrawMode(static_cast<MHWRender::MGeometry::DrawMode>(
        MHWRender::MGeometry::kShaded | MHWRender::MGeometry::kTextured));
    renderItem->castsShadows(false);
    renderItem->receivesShadows(false);
    renderItem->setShader(_delegate->GetPointsFallbackShader(MColor()));
    _InitRenderItemCommon(renderItem);

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
    MSelectionMask selectionMask(MSelectionMask::kSelectParticleShapes);
    selectionMask.addMask(MSelectionMask::kSelectPointsForGravity);
    renderItem->setSelectionMask(selectionMask);
#else
    renderItem->setSelectionMask(MSelectionMask::kSelectParticleShapes);
#endif

    renderItem->setObjectTypeExclusionFlag(MHWRender::MFrameContext::kExcludeNParticles);

    return renderItem;
}

//! \brief  Update the named repr object for this Rprim.
void HdVP2GaussianSplat::_UpdateRepr(
    PXR_NS::HdSceneDelegate* sceneDelegate,
    const PXR_NS::TfToken&   reprToken)
{
    PXR_NS::HdReprSharedPtr const& repr = _GetRepr(reprToken);
    if (!repr) {
        return;
    }

    const size_t numDrawItems = repr->GetDrawItems().size();
    for (size_t i = 0; i < numDrawItems; ++i) {
        PXR_NS::HdVP2DrawItem* drawItem = static_cast<PXR_NS::HdVP2DrawItem*>(repr->GetDrawItem(i));
        if (drawItem) {
            _UpdateDrawItem(sceneDelegate, drawItem);
        }
    }
}

/*! \brief  Update the draw item

    This call happens on worker threads and results of the change are collected
    in MayaUsdCommitState and enqueued for Commit on main-thread using CommitTasks
*/
void HdVP2GaussianSplat::_UpdateDrawItem(
    PXR_NS::HdSceneDelegate* sceneDelegate,
    PXR_NS::HdVP2DrawItem*   drawItem)
{
    if (drawItem->GetRenderItems().empty()) {
        return;
    }

    MHWRender::MRenderItem* renderItem = drawItem->GetRenderItem();
    PXR_NS::HdDirtyBits     itemDirtyBits = drawItem->GetDirtyBits();

    PXR_NS::MayaUsdCommitState             stateToCommit(drawItem->GetRenderItemData());
    PXR_NS::HdVP2DrawItem::RenderItemData& drawItemData = stateToCommit._renderItemData;

    auto* const param = static_cast<PXR_NS::HdVP2RenderParam*>(_delegate->GetRenderParam());
    PXR_NS::ProxyRenderDelegate& drawScene = param->GetDrawScene();

    const bool inTemplateMode = _displayLayerModes._displayType == PXR_NS::MayaUsdRPrim::kTemplate;
    const bool inReferenceMode
        = _displayLayerModes._displayType == PXR_NS::MayaUsdRPrim::kReference;

    // Like MayaUSD points, selected splats are drawn with the highlight color
    // instead of their own color.
    if (itemDirtyBits & (DirtySelectionHighlight | PXR_NS::HdChangeTracker::DirtyDisplayStyle)) {
        MHWRender::MShaderInstance* shader = _delegate->Get3dCPVFatPointShader();
        bool                        isTransparent = false;
        if (_selectionStatus != kUnselected || inTemplateMode) {
            const MColor color = _GetHighlightColor(PXR_NS::HdPrimTypeTokens->points);
            shader = _delegate->Get3dFatPointShader(color);
            isTransparent = (color.a < kOpaqueAlphaThreshold);
        }

        if (shader != nullptr && shader != drawItemData._shader) {
            drawItemData._shader = shader;
            stateToCommit._shader = shader;
            stateToCommit._isTransparent = isTransparent;
        }
    }

    _UpdateTransform(stateToCommit, _sharedData, itemDirtyBits, /*isBoundingBoxItem*/ false);

    // Determine if the render item should be enabled or not.
    if (itemDirtyBits
        & (PXR_NS::HdChangeTracker::DirtyVisibility | PXR_NS::HdChangeTracker::DirtyRenderTag
           | PXR_NS::HdChangeTracker::DirtyPoints | PXR_NS::HdChangeTracker::DirtyPrimvar
           | PXR_NS::HdChangeTracker::DirtyExtent | DirtySelectionHighlight)) {
        // Hide the item when there are no splats.
        bool enable = drawItem->GetVisible() && !_splatSharedData._points.empty();
        enable = enable && drawScene.DrawRenderTag(_splatSharedData._renderTag);

        if (drawItemData._enabled != enable) {
            drawItemData._enabled = enable;
            stateToCommit._enabled = &drawItemData._enabled;
        }
    }

    stateToCommit._geometryDirty
        = (itemDirtyBits
           & (PXR_NS::HdChangeTracker::DirtyPoints | PXR_NS::HdChangeTracker::DirtyPrimvar
              | DirtySelectionHighlight));

    // update selection mask if dirty
    if (itemDirtyBits & DirtySelectionHighlight) {
        MSelectionMask selectionMask(MSelectionMask::kSelectParticleShapes);

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
        // Only unselected Rprims can be used for point snapping.
        if (param->WantsSelectPointsForGravity() && _selectionStatus == kUnselected) {
            selectionMask.addMask(MSelectionMask::kSelectPointsForGravity);
        }
#endif
        // In template and reference mode, items should have no selection
        if (inTemplateMode || inReferenceMode) {
            selectionMask = MSelectionMask();
        }

        // The function is thread-safe, thus called in place to keep simple.
        renderItem->setSelectionMask(selectionMask);
    }

    // Reset dirty bits because we've prepared commit state for this draw item.
    drawItem->ResetDirtyBits();

    MHWRender::MVertexBuffer* positionsBuffer = _splatSharedData._positionsBuffer.get();
    MHWRender::MVertexBuffer* colorBuffer = _splatSharedData._colorBuffer.get();
    MHWRender::MIndexBuffer*  indexBuffer = drawItemData._indexBuffer.get();

    _delegate->GetVP2ResourceRegistry().EnqueueCommit(
        [drawItem, stateToCommit, param, positionsBuffer, colorBuffer, indexBuffer]() {
            MHWRender::MRenderItem* renderItem = drawItem->GetRenderItem();
            if (ARCH_UNLIKELY(!renderItem)) {
                return;
            }

            if (stateToCommit._shader != nullptr) {
                renderItem->setShader(stateToCommit._shader);
                renderItem->setTreatAsTransparent(stateToCommit._isTransparent);
            }

            if (stateToCommit._enabled != nullptr) {
                renderItem->enable(*stateToCommit._enabled);
            }

            PXR_NS::ProxyRenderDelegate& drawScene = param->GetDrawScene();

            if ((stateToCommit._geometryDirty || stateToCommit._boundingBox)) {
                MHWRender::MVertexBufferArray vertexBuffers;
                vertexBuffers.addBuffer(kPositionsStr, positionsBuffer);

                if (colorBuffer) {
                    vertexBuffers.addBuffer(kDiffuseColorStr, colorBuffer);
                }

                drawScene.setGeometryForRenderItem(
                    *renderItem, vertexBuffers, *indexBuffer, stateToCommit._boundingBox);
            }

            if (stateToCommit._worldMatrix != nullptr) {
                renderItem->setMatrix(stateToCommit._worldMatrix);
            }
        });
}

} // namespace MAYAUSD_NS_DEF

#endif // PXR_VERSION >= 2605
