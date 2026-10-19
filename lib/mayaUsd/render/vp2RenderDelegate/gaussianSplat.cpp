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

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

//! Factor converting the first SH coefficient to a color: rgb = 0.5 + kSH_C0 * coeff.
constexpr float kSH_C0 = 0.28209479177387814f;

//! Splats below this opacity would not contribute a single 8-bit color step, so they
//! are left out of the index buffer.
constexpr float kMinVisibleOpacity = 1.0f / 255.0f;

//! Highlight colors with alpha below this are drawn as transparent.
constexpr float kOpaqueAlphaThreshold = 0.999f;

//! Positions and colors can be authored in float or half precision.
bool _GetVec3fArray(const VtValue& value, VtVec3fArray& result)
{
    const VtValue cast = VtValue::Cast<VtVec3fArray>(value);
    if (cast.IsEmpty()) {
        return false;
    }
    result = cast.UncheckedGet<VtVec3fArray>();
    return true;
}

//! Opacities can be authored in float or half precision.
bool _GetFloatArray(const VtValue& value, VtFloatArray& result)
{
    const VtValue cast = VtValue::Cast<VtFloatArray>(value);
    if (cast.IsEmpty()) {
        return false;
    }
    result = cast.UncheckedGet<VtFloatArray>();
    return true;
}

} // anonymous namespace

//! \brief  Constructor
HdVP2GaussianSplat::HdVP2GaussianSplat(
    HdVP2RenderDelegate* delegate,
    const SdfPath&       id
#if defined(HD_API_VERSION) && HD_API_VERSION >= 36
    )
    : HdRprim(id)
#else
    ,
    const SdfPath& instancerId)
    : HdRprim(id, instancerId)
#endif
    , MayaUsdRPrim(delegate, id)
{
    const MHWRender::MVertexBufferDescriptor desc(
        "", MHWRender::MGeometry::kPosition, MHWRender::MGeometry::kFloat, 3);

    _splatSharedData._positionsBuffer.reset(new MHWRender::MVertexBuffer(desc));
}

//! \brief  Synchronize VP2 state with scene delegate state based on dirty bits and repr
void HdVP2GaussianSplat::Sync(
    HdSceneDelegate* delegate,
    HdRenderParam*   renderParam,
    HdDirtyBits*     dirtyBits,
    const TfToken&   reprToken)
{
    if (!_SyncCommon(*this, delegate, renderParam, dirtyBits, _GetRepr(reprToken), reprToken)) {
        return;
    }

    MProfilingScope profilingScope(
        HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorC_L2,
        _rprimId.asChar(),
        "HdVP2GaussianSplat::Sync");

    // All splat attributes (positions, opacities, SH coefficients) are primvars.
    if (*dirtyBits & HdChangeTracker::DirtyPrimvar) {
        const SdfPath& id = GetId();

        if (_GetVec3fArray(delegate->Get(id, UsdVolTokens->positions), _splatSharedData._points)) {
            _CommitPositionsBuffer(
                _splatSharedData._positionsBuffer.get(), _splatSharedData._points);
        } else {
            TF_WARN("Gaussian splat %s has no positions.", id.GetText());
            _splatSharedData._points.clear();
        }

        const size_t numSplats = _splatSharedData._points.size();

        // Splats below kMinVisibleOpacity are dropped from the index buffer, not drawn.
        VtFloatArray& opacities = _splatSharedData._opacities;
        if (_GetFloatArray(delegate->Get(id, UsdVolTokens->opacities), opacities)
            && opacities.size() == numSplats) {
            _splatSharedData._numVisibleSplats = static_cast<unsigned int>(
                std::count_if(opacities.cbegin(), opacities.cend(), [](float opacity) {
                    return opacity >= kMinVisibleOpacity;
                }));
        } else {
            // No usable opacities: all splats are visible.
            opacities.assign(numSplats, 1.0f);
            _splatSharedData._numVisibleSplats = static_cast<unsigned int>(numSplats);
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
void HdVP2GaussianSplat::_UpdateColorBuffer(HdSceneDelegate* sceneDelegate)
{
    const SdfPath& id = GetId();
    const size_t   numSplats = _splatSharedData._points.size();
    if (numSplats == 0) {
        return;
    }

    // The coefficients are stored splat by splat, shStride coefficients per splat.
    VtVec3fArray shCoeffs;
    _GetVec3fArray(
        sceneDelegate->Get(id, UsdVolTokens->radianceSphericalHarmonicsCoefficients), shCoeffs);
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
            const GfVec3f color = GfVec3f(0.5f) + kSH_C0 * shCoeffs[i * shStride];
            // SH can learn values that give out of range colors, clamp them.
            bufferData[offset++] = GfClamp(color[0], 0.0f, 1.0f);
            bufferData[offset++] = GfClamp(color[1], 0.0f, 1.0f);
            bufferData[offset++] = GfClamp(color[2], 0.0f, 1.0f);
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

        auto* const   param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
        const GfVec3f defaultColor
            = param->GetDrawScene().GetDefaultColor(HdPrimTypeTokens->points);

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
const TfTokenVector& HdVP2GaussianSplat::GetBuiltinPrimvarNames() const
{
    static const TfTokenVector primvarNames
        = { UsdVolTokens->positions,
            UsdVolTokens->opacities,
            UsdVolTokens->radianceSphericalHarmonicsCoefficients };
    return primvarNames;
}

/*! \brief  Returns the minimal set of dirty bits to place in the
            change tracker for use in the first sync of this prim.
*/
HdDirtyBits HdVP2GaussianSplat::GetInitialDirtyBitsMask() const
{
    constexpr HdDirtyBits bits = HdChangeTracker::InitRepr | HdChangeTracker::DirtyExtent
        | HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyPrimID
        | HdChangeTracker::DirtyPrimvar | HdChangeTracker::DirtyDisplayStyle
        | HdChangeTracker::DirtyRepr | HdChangeTracker::DirtyMaterialId
        | HdChangeTracker::DirtyTransform | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyRenderTag | DirtySelectionHighlight;

    return bits;
}

//! \brief  Add additional dirty bits
HdDirtyBits HdVP2GaussianSplat::_PropagateDirtyBits(HdDirtyBits bits) const
{
    _PropagateDirtyBitsCommon(bits, _reprs);
    return bits;
}

//! \brief  Initialize the given representation of this Rprim.
void HdVP2GaussianSplat::_InitRepr(const TfToken& reprToken, HdDirtyBits* dirtyBits)
{
    auto* const         param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    MSubSceneContainer* subSceneContainer = param->GetContainer();
    if (ARCH_UNLIKELY(!subSceneContainer)) {
        return;
    }

    HdReprSharedPtr repr = _InitReprCommon(*this, reprToken, _reprs, dirtyBits, GetId());
    if (!repr) {
        return;
    }

#if HD_API_VERSION < 35
    auto* drawItem = new HdVP2DrawItem(_delegate, &_sharedData);
#else
    std::unique_ptr<HdVP2DrawItem> drawItem
        = std::make_unique<HdVP2DrawItem>(_delegate, &_sharedData);
#endif

    MHWRender::MRenderItem* renderItem = _CreateSplatRenderItem(drawItem->GetDrawItemName());
    drawItem->AddUsage(HdVP2DrawItem::kSelectionHighlight);
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
void HdVP2GaussianSplat::_UpdateRepr(HdSceneDelegate* sceneDelegate, const TfToken& reprToken)
{
    HdReprSharedPtr const& repr = _GetRepr(reprToken);
    if (!repr) {
        return;
    }

    const size_t numDrawItems = repr->GetDrawItems().size();
    for (size_t i = 0; i < numDrawItems; ++i) {
        HdVP2DrawItem* drawItem = static_cast<HdVP2DrawItem*>(repr->GetDrawItem(i));
        if (drawItem) {
            _UpdateDrawItem(sceneDelegate, drawItem);
        }
    }
}

//! \brief  Fill the index buffer with the indices of the visible splats.
void HdVP2GaussianSplat::_UpdateIndexBuffer(
    HdVP2DrawItem::RenderItemData& drawItemData,
    MayaUsdCommitState&            stateToCommit) const
{
    TRACE_FUNCTION();

    const unsigned int numVisibleSplats = _splatSharedData._numVisibleSplats;
    if (numVisibleSplats == 0) {
        return;
    }

    int* indexData = static_cast<int*>(drawItemData._indexBuffer->acquire(numVisibleSplats, true));
    if (!indexData) {
        return;
    }

    const VtFloatArray& opacities = _splatSharedData._opacities;
    unsigned int        numIndices = 0;
    for (size_t i = 0; i < opacities.size(); ++i) {
        if (opacities[i] >= kMinVisibleOpacity) {
            indexData[numIndices++] = static_cast<int>(i);
        }
    }

    stateToCommit._indexBufferData = indexData;
    drawItemData._indexBufferValid = true;
}

/*! \brief  Update the draw item

    This call happens on worker threads and results of the change are collected
    in MayaUsdCommitState and enqueued for Commit on main-thread using CommitTasks
*/
void HdVP2GaussianSplat::_UpdateDrawItem(HdSceneDelegate* sceneDelegate, HdVP2DrawItem* drawItem)
{
    if (drawItem->GetRenderItems().empty()) {
        return;
    }

    MHWRender::MRenderItem* renderItem = drawItem->GetRenderItem();
    HdDirtyBits             itemDirtyBits = drawItem->GetDirtyBits();

    MayaUsdCommitState             stateToCommit(drawItem->GetRenderItemData());
    HdVP2DrawItem::RenderItemData& drawItemData = stateToCommit._renderItemData;

    auto* const          param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    ProxyRenderDelegate& drawScene = param->GetDrawScene();

    const bool inTemplateMode = _displayLayerModes._displayType == MayaUsdRPrim::kTemplate;
    const bool inReferenceMode = _displayLayerModes._displayType == MayaUsdRPrim::kReference;

    // Like MayaUSD points, selected splats are drawn with the highlight color
    // instead of their own color.
    if (itemDirtyBits & (DirtySelectionHighlight | HdChangeTracker::DirtyDisplayStyle)) {
        MHWRender::MShaderInstance* shader = _delegate->Get3dCPVFatPointShader();
        bool                        isTransparent = false;
        if (_selectionStatus != kUnselected || inTemplateMode) {
            const MColor color = _GetHighlightColor(HdPrimTypeTokens->points);
            shader = _delegate->Get3dFatPointShader(color);
            isTransparent = (color.a < kOpaqueAlphaThreshold);
        }

        if (shader != nullptr && shader != drawItemData._shader) {
            drawItemData._shader = shader;
            stateToCommit._shader = shader;
            stateToCommit._isTransparent = isTransparent;
        }
    }

    if (itemDirtyBits & (HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyPrimvar)) {
        _UpdateIndexBuffer(drawItemData, stateToCommit);
    }

    _UpdateTransform(stateToCommit, _sharedData, itemDirtyBits, /*isBoundingBoxItem*/ false);

    // Determine if the render item should be enabled or not.
    if (itemDirtyBits
        & (HdChangeTracker::DirtyVisibility | HdChangeTracker::DirtyRenderTag
           | HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyPrimvar
           | HdChangeTracker::DirtyExtent | DirtySelectionHighlight)) {
        // Hide the item when no splat is visible, since its index buffer is not updated.
        bool enable = drawItem->GetVisible() && _splatSharedData._numVisibleSplats > 0;
        enable = enable && drawScene.DrawRenderTag(_splatSharedData._renderTag);

        if (drawItemData._enabled != enable) {
            drawItemData._enabled = enable;
            stateToCommit._enabled = &drawItemData._enabled;
        }
    }

    stateToCommit._geometryDirty
        = (itemDirtyBits
           & (HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyPrimvar
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
    const bool                indexBufferValid = drawItemData._indexBufferValid;

    _delegate->GetVP2ResourceRegistry().EnqueueCommit([drawItem,
                                                       stateToCommit,
                                                       param,
                                                       positionsBuffer,
                                                       colorBuffer,
                                                       indexBuffer,
                                                       indexBufferValid]() {
        MHWRender::MRenderItem* renderItem = drawItem->GetRenderItem();
        if (ARCH_UNLIKELY(!renderItem)) {
            return;
        }

        if (stateToCommit._indexBufferData) {
            indexBuffer->commit(stateToCommit._indexBufferData);
        }

        if (stateToCommit._shader != nullptr) {
            renderItem->setShader(stateToCommit._shader);
            renderItem->setTreatAsTransparent(stateToCommit._isTransparent);
        }

        if (stateToCommit._enabled != nullptr) {
            renderItem->enable(*stateToCommit._enabled);
        }

        ProxyRenderDelegate& drawScene = param->GetDrawScene();

        if ((stateToCommit._geometryDirty || stateToCommit._boundingBox) && indexBufferValid) {
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

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2605
