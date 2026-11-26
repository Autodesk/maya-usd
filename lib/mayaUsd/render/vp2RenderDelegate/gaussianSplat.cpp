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
#include "tokens.h"

#include <pxr/base/arch/hints.h>
#include <pxr/base/gf/math.h>
#include <pxr/base/gf/quatf.h>
#include <pxr/base/vt/array.h>
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

//! Scale of a splat whose prim has no usable scales: the semi-axes of its ellipsoid, in the
//! prim's local space.
constexpr float kDefaultScale = 0.05f;

//! Keys of the packed float4 streams. The names are only keys, the shader pairs the streams
//! with its inputs by bind order against the alphabetical order of the input semantic names
//! (splatColor, splatOrientation, splatScaleOpacity).
const MString kSplatColorStr("splatColor");
const MString kSplatOrientationStr("splatOrientation");
const MString kSplatScaleOpacityStr("splatScaleOpacity");

/*! \brief  Acquire numSplats float4 values of a stream for writing, creating the stream first if
            it does not exist. The kTexture semantic is required, see the shared data.

    \return null when the stream cannot be acquired.
*/
float* _AcquireFloat4Stream(std::unique_ptr<MHWRender::MVertexBuffer>& buffer, size_t numSplats)
{
    if (!buffer) {
        const MHWRender::MVertexBufferDescriptor desc(
            "", MHWRender::MGeometry::kTexture, MHWRender::MGeometry::kFloat, 4);
        buffer.reset(new MHWRender::MVertexBuffer(desc));
    }
    return static_cast<float*>(buffer->acquire(numSplats, true));
}

/*! \brief  Read a per-splat array that must have one value per splat.

    \return false when the array is not authored, or has the wrong type or size, in which case the
            caller uses the default values. A wrong type or size is reported with a warning.
*/
template <typename ArrayT>
bool _GetPerSplatArray(
    PXR_NS::HdSceneDelegate* sceneDelegate,
    const PXR_NS::SdfPath&   id,
    const PXR_NS::TfToken&   name,
    const size_t             numSplats,
    ArrayT&                  result)
{
    const PXR_NS::VtValue value = sceneDelegate->Get(id, name);
    if (value.IsEmpty()) {
        return false;
    }

    const PXR_NS::VtValue cast = PXR_NS::VtValue::Cast<ArrayT>(value);
    if (cast.IsEmpty()) {
        TF_WARN(
            "Gaussian splat %s: %s has an unsupported type, using defaults.",
            id.GetText(),
            name.GetText());
        return false;
    }

    result = cast.UncheckedGet<ArrayT>();
    if (result.empty()) {
        return false;
    }
    if (result.size() != numSplats) {
        TF_WARN(
            "Gaussian splat %s: %s has %zu values for %zu splats, using defaults.",
            id.GetText(),
            name.GetText(),
            result.size(),
            numSplats);
        result = ArrayT();
        return false;
    }
    return true;
}

//! Positions can be authored in float or half precision.
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
    , _resources(delegate)
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

    // Sync instance topology if necessary.
    _UpdateInstancer(delegate, dirtyBits);

    // Clone the shader templates on the first Sync: the plain pair, or the instanced pair when the
    // Rprim has an instancer. Its instanced state never changes.
    _resources.CreateShaders(!GetInstancerId().IsEmpty());

    // All splat attributes (positions, opacities, scales, orientations and SH coefficients) are
    // primvars. They are read once per sync batch, not once per repr token.
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

        const HdVP2GaussianSplatSHCoefficients sh
            = HdVP2GaussianSplatReadSHCoefficients(delegate, id, _splatSharedData._points.size());
        _UpdateColorBuffer(sh);
        _resources.UpdateSHTexture(id, sh, _splatSharedData._points.size());
        _UpdateOrientationScaleOpacityBuffers(delegate);
    }

    _SyncEndCommon(*this, delegate, dirtyBits, reprToken, _sharedData, _reprs);

    // Draw item update is controlled by its own dirty bits.
    _UpdateRepr(delegate, reprToken);
}

/*! \brief  Fill the color buffer from the first spherical harmonics coefficient of each splat.

    Splats are drawn opaque, so alpha is always 1. This is the color of the splats when the SH
    texture is not used (degree 0, or the fallback).
*/
void HdVP2GaussianSplat::_UpdateColorBuffer(const HdVP2GaussianSplatSHCoefficients& sh)
{
    const PXR_NS::SdfPath& id = GetId();
    const size_t           numSplats = _splatSharedData._points.size();
    if (numSplats == 0) {
        return;
    }

    float* const bufferData = _AcquireFloat4Stream(_splatSharedData._colorBuffer, numSplats);
    if (!bufferData) {
        return;
    }

    PXR_NS::GfVec3f defaultColor(0.0f);
    if (!sh._valid) {
        // Without usable SH coefficients every splat gets the default color.
        TF_WARN(
            "Gaussian splat %s: missing or mismatched SH coefficients (%zu for %zu splats), "
            "using default color.",
            id.GetText(),
            sh._values.size(),
            numSplats);

        auto* const param = static_cast<PXR_NS::HdVP2RenderParam*>(_delegate->GetRenderParam());
        defaultColor = param->GetDrawScene().GetDefaultColor(PXR_NS::HdPrimTypeTokens->points);
    }
    HdVP2GaussianSplatFillSH0Colors(sh, numSplats, defaultColor, bufferData);

    _CommitMVertexBuffer(_splatSharedData._colorBuffer.get(), bufferData);
}

/*! \brief  Pack the orientations, scales and opacities into their float4 streams.

    A missing input, or one that does not have one value per splat, uses its default: identity
    rotation, a fixed scale and opacity 1.
*/
void HdVP2GaussianSplat::_UpdateOrientationScaleOpacityBuffers(
    PXR_NS::HdSceneDelegate* sceneDelegate)
{
    const PXR_NS::SdfPath& id = GetId();
    const size_t           numSplats = _splatSharedData._points.size();
    if (numSplats == 0) {
        return;
    }

    PXR_NS::VtFloatArray opacities;
    PXR_NS::VtVec3fArray scales;
    PXR_NS::VtQuatfArray orientations;
    const bool           hasOpacities = _GetPerSplatArray(
        sceneDelegate, id, PXR_NS::UsdVolTokens->opacities, numSplats, opacities);
    const bool hasScales
        = _GetPerSplatArray(sceneDelegate, id, PXR_NS::UsdVolTokens->scales, numSplats, scales);
    const bool hasOrientations = _GetPerSplatArray(
        sceneDelegate, id, PXR_NS::UsdVolTokens->orientations, numSplats, orientations);

    float* const orientationData
        = _AcquireFloat4Stream(_splatSharedData._orientationBuffer, numSplats);
    if (orientationData) {
        for (size_t i = 0; i < numSplats; ++i) {
            float* const out = orientationData + 4 * i;
            if (hasOrientations) {
                const PXR_NS::GfQuatf& q = orientations[i];
                const PXR_NS::GfVec3f& im = q.GetImaginary();
                out[0] = im[0];
                out[1] = im[1];
                out[2] = im[2];
                out[3] = q.GetReal();
            } else {
                out[0] = out[1] = out[2] = 0.0f;
                out[3] = 1.0f;
            }
        }
        _CommitMVertexBuffer(_splatSharedData._orientationBuffer.get(), orientationData);
    }

    float* const scaleOpacityData
        = _AcquireFloat4Stream(_splatSharedData._scaleOpacityBuffer, numSplats);
    if (scaleOpacityData) {
        for (size_t i = 0; i < numSplats; ++i) {
            float* const out = scaleOpacityData + 4 * i;
            if (hasScales) {
                out[0] = scales[i][0];
                out[1] = scales[i][1];
                out[2] = scales[i][2];
            } else {
                out[0] = out[1] = out[2] = kDefaultScale;
            }
            out[3] = hasOpacities ? PXR_NS::GfClamp(opacities[i], 0.0f, 1.0f) : 1.0f;
        }
        _CommitMVertexBuffer(_splatSharedData._scaleOpacityBuffer.get(), scaleOpacityData);
    }
}

/*! \brief  Returns the names of built-in primvars, i.e. primvars that are part of the core
            geometric schema for this prim.
*/
const PXR_NS::TfTokenVector& HdVP2GaussianSplat::GetBuiltinPrimvarNames() const
{
    static const PXR_NS::TfTokenVector primvarNames
        = { PXR_NS::UsdVolTokens->positions,
            PXR_NS::UsdVolTokens->opacities,
            PXR_NS::UsdVolTokens->scales,
            PXR_NS::UsdVolTokens->orientations,
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
        | PXR_NS::HdChangeTracker::DirtyInstancer | DirtySelectionHighlight;

    return bits;
}

//! \brief  Add additional dirty bits
PXR_NS::HdDirtyBits HdVP2GaussianSplat::_PropagateDirtyBits(PXR_NS::HdDirtyBits bits) const
{
    _PropagateDirtyBitsCommon(bits, _reprs);
    return bits;
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
            _UpdateDrawItem(sceneDelegate, drawItem, reprToken);
        }
    }
}

/*! \brief  Update the draw item

    This call happens on worker threads and results of the change are collected
    in MayaUsdCommitState and enqueued for Commit on main-thread using CommitTasks.
    Every Maya call is made in the commit lambda.

    An Rprim with an instancer draws through the instanced clone of the shader, with one instance
    matrix per instance that no display layer drops, and one tint per instance. An Rprim without
    one draws through the plain clone, with the world matrix on the item and the tint as a shader
    parameter.
*/
void HdVP2GaussianSplat::_UpdateDrawItem(
    PXR_NS::HdSceneDelegate* sceneDelegate,
    PXR_NS::HdVP2DrawItem*   drawItem,
    const PXR_NS::TfToken&   reprToken)
{
    if (drawItem->GetRenderItems().empty()) {
        return;
    }

    // Only the shaded reprs and the wireframe repr have items.
    HdVP2GaussianSplatItemDesc itemDesc;
    if (!HdVP2GaussianSplatGetItemDesc(reprToken, itemDesc)) {
        return;
    }
    const bool isWire = itemDesc._isWire;

    MHWRender::MRenderItem* renderItem = drawItem->GetRenderItem();
    PXR_NS::HdDirtyBits     itemDirtyBits = drawItem->GetDirtyBits();

    PXR_NS::MayaUsdCommitState             stateToCommit(drawItem->GetRenderItemData());
    PXR_NS::HdVP2DrawItem::RenderItemData& drawItemData = stateToCommit._renderItemData;

    auto* const param = static_cast<PXR_NS::HdVP2RenderParam*>(_delegate->GetRenderParam());
    PXR_NS::ProxyRenderDelegate& drawScene = param->GetDrawScene();

    const bool inTemplateMode = _displayLayerModes._displayType == PXR_NS::MayaUsdRPrim::kTemplate;
    const bool inReferenceMode
        = _displayLayerModes._displayType == PXR_NS::MayaUsdRPrim::kReference;

    const bool instanced = !GetInstancerId().IsEmpty();

    // The shaded items share the Rprim's shader clone, the wire item has its own. They are the
    // plain or the instanced version, as chosen at the first Sync.
    MHWRender::MShaderInstance* const shader = _resources.GetShader(isWire, instanced);
    bool                              shaderChanged = false;
    if (shader != nullptr && shader != drawItemData._shader) {
        drawItemData._shader = shader;
        stateToCommit._shader = shader;
        shaderChanged = true;
    }

    _UpdateTransform(stateToCommit, _sharedData, itemDirtyBits, /*isBoundingBoxItem*/ false);

    // Instance matrices, tints and identifiers.
    const HdVP2GaussianSplatInstancing instancing
        = _UpdateInstancing(sceneDelegate, drawItem, reprToken, isWire, stateToCommit);

    // Determine if the render item should be enabled or not.
    if (instanced
        || (itemDirtyBits
            & (PXR_NS::HdChangeTracker::DirtyVisibility | PXR_NS::HdChangeTracker::DirtyRenderTag
               | PXR_NS::HdChangeTracker::DirtyPoints | PXR_NS::HdChangeTracker::DirtyPrimvar
               | PXR_NS::HdChangeTracker::DirtyExtent | DirtySelectionHighlight
               | PXR_NS::MayaUsdRPrim::DirtyDisplayLayers))) {
        // Hide the item when there are no splats.
        bool enable = drawItem->GetVisible() && !_splatSharedData._points.empty()
            && !instancing._noInstances;
        enable = enable && drawScene.DrawRenderTag(_splatSharedData._renderTag);
        // A display layer override (wireframe, bounding box) disabled the shaded items. Do not
        // enable them again while the override lasts. The wire item is the one that draws under
        // a wireframe override, and a bounding box override hides it too.
        enable = enable
            && (isWire ? _reprOverride != PXR_NS::MayaUsdRPrim::kBBox
                       : _reprOverride == PXR_NS::MayaUsdRPrim::kNone);

        if (drawItemData._enabled != enable) {
            drawItemData._enabled = enable;
            stateToCommit._enabled = &drawItemData._enabled;
        }
    }

    // A new shader binds the geometry again.
    stateToCommit._geometryDirty
        = (itemDirtyBits
           & (PXR_NS::HdChangeTracker::DirtyPoints | PXR_NS::HdChangeTracker::DirtyPrimvar))
        || shaderChanged;

    // Selection tint of the plain shaded items: the selection color blended at a small factor, the
    // lead color for the lead selection and the active color for other selected prims. Alpha 0
    // means no tint. The wire item keeps the splat color. Instanced items tint per instance only,
    // the whole-prim tint does not apply to them.
    bool  tintDirty = false;
    float tint[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    if (!isWire && !instanced
        && (itemDirtyBits & (DirtySelectionHighlight | PXR_NS::HdChangeTracker::DirtyRepr))) {
        tintDirty = true;
        if (_selectionStatus != kUnselected && !inTemplateMode && !inReferenceMode) {
            const MColor color = _GetHighlightColor(PXR_NS::HdPrimTypeTokens->points);
            tint[0] = color.r;
            tint[1] = color.g;
            tint[2] = color.b;
            tint[3] = kHdVP2GaussianSplatSelectionTintAlpha;
        }
    }

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
    MHWRender::MVertexBuffer* orientationBuffer = _splatSharedData._orientationBuffer.get();
    MHWRender::MVertexBuffer* scaleOpacityBuffer = _splatSharedData._scaleOpacityBuffer.get();
    MHWRender::MIndexBuffer*  indexBuffer = drawItemData._indexBuffer.get();

    _delegate->GetVP2ResourceRegistry().EnqueueCommit([this,
                                                       drawItem,
                                                       stateToCommit,
                                                       param,
                                                       positionsBuffer,
                                                       colorBuffer,
                                                       orientationBuffer,
                                                       scaleOpacityBuffer,
                                                       indexBuffer,
                                                       shader,
                                                       tintDirty,
                                                       tint,
                                                       isWire,
                                                       instancing]() {
        MHWRender::MRenderItem* renderItem = drawItem->GetRenderItem();
        if (ARCH_UNLIKELY(!renderItem)) {
            return;
        }

        if (stateToCommit._shader != nullptr) {
            renderItem->setShader(stateToCommit._shader);
            // Splat items are never transparent: opacity is stochastic.
            renderItem->setTreatAsTransparent(false);
        }

        if (tintDirty && shader) {
            shader->setParameter(PXR_NS::MayaUsdRPrim::kSolidColorStr, tint);
        }

        if (stateToCommit._enabled != nullptr) {
            renderItem->enable(*stateToCommit._enabled);
        }

        PXR_NS::ProxyRenderDelegate& drawScene = param->GetDrawScene();

        if ((stateToCommit._geometryDirty || stateToCommit._boundingBox) && positionsBuffer
            && colorBuffer && orientationBuffer && scaleOpacityBuffer) {
            // The position stream is bound first, then the float4 streams in the alphabetical
            // order of the semantic names of the geometry fragment inputs. The index buffer is
            // empty, so the draw is non-indexed and the primitive ID is the splat number.
            MHWRender::MVertexBufferArray vertexBuffers;
            vertexBuffers.addBuffer(PXR_NS::MayaUsdRPrim::kPositionsStr, positionsBuffer);
            vertexBuffers.addBuffer(kSplatColorStr, colorBuffer);
            vertexBuffers.addBuffer(kSplatOrientationStr, orientationBuffer);
            vertexBuffers.addBuffer(kSplatScaleOpacityStr, scaleOpacityBuffer);

            drawScene.setGeometryForRenderItem(
                *renderItem, vertexBuffers, *indexBuffer, stateToCommit._boundingBox);
        }

        // Important, update instance transforms after setting geometry on render items!
        HdVP2GaussianSplatCommitInstancing(
            drawScene, *renderItem, stateToCommit, instancing, isWire, GetId());
    });
}

} // namespace MAYAUSD_NS_DEF

#endif // PXR_VERSION >= 2605
