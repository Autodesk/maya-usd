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

#include "bboxGeom.h"
#include "drawItem.h"
#include "renderDelegate.h"
#include "tokens.h"

#include <pxr/base/arch/hints.h>
#include <pxr/base/gf/half.h>
#include <pxr/base/gf/math.h>
#include <pxr/base/gf/quatf.h>
#include <pxr/base/gf/quath.h>
#include <pxr/base/gf/vec3h.h>
#include <pxr/base/trace/trace.h>
#include <pxr/base/vt/types.h>
#include <pxr/base/vt/value.h>
#include <pxr/imaging/hd/repr.h>
#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdVol/tokens.h>

#include <maya/MProfiler.h>
#include <maya/MSelectionMask.h>
#include <maya/MShaderManager.h>
#include <maya/MViewport2Renderer.h>

#include <algorithm>
#include <numeric>

namespace MAYAUSD_NS_DEF {

namespace {

//! Default shape of a splat: unit scale, identity orientation (x, y, z, w) and full opacity. These
//! are the schema fallbacks, and apply when the attribute is missing or too short.
constexpr float kDefaultScale = 1.0f;
constexpr float kDefaultOpacity = 1.0f;

//! The schema default of the spherical harmonics degree.
constexpr int kDefaultShDegree = 3;

//! Highlight colors with alpha below this make the splats translucent (alpha multiplies the
//! opacity of each splat).
constexpr float kOpaqueAlphaThreshold = 0.999f;

//! Names of the per-splat vertex streams bound to the render item (consumed by the shader
//! fragments).
constexpr const char* kSplatScaleStream = "splatScale";
constexpr const char* kSplatColorStream = "splatColor";
constexpr const char* kSplatOrientationStream = "splatOrientation";
constexpr const char* kSplatOpacityStream = "splatOpacity";

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

//! Opacities can be authored in float or half precision.
bool _GetFloatArray(const PXR_NS::VtValue& value, PXR_NS::VtFloatArray& result)
{
    const PXR_NS::VtValue cast = PXR_NS::VtValue::Cast<PXR_NS::VtFloatArray>(value);
    if (cast.IsEmpty()) {
        return false;
    }
    result = cast.UncheckedGet<PXR_NS::VtFloatArray>();
    return true;
}

//! Orientations can be authored in float or half precision. Vt has no cast from quath to quatf,
//! so each component is converted here. The result is stored x, y, z, w.
bool _GetOrientationArray(const PXR_NS::VtValue& value, PXR_NS::VtArray<PXR_NS::GfVec4f>& result)
{
    if (value.IsHolding<PXR_NS::VtQuatfArray>()) {
        const PXR_NS::VtQuatfArray& quats = value.UncheckedGet<PXR_NS::VtQuatfArray>();
        result.resize(quats.size());
        for (size_t i = 0; i < quats.size(); ++i) {
            const PXR_NS::GfVec3f& im = quats[i].GetImaginary();
            result[i] = PXR_NS::GfVec4f(im[0], im[1], im[2], quats[i].GetReal());
        }
        return true;
    }
    if (value.IsHolding<PXR_NS::VtQuathArray>()) {
        const PXR_NS::VtQuathArray& quats = value.UncheckedGet<PXR_NS::VtQuathArray>();
        result.resize(quats.size());
        for (size_t i = 0; i < quats.size(); ++i) {
            const PXR_NS::GfVec3h& im = quats[i].GetImaginary();
            result[i] = PXR_NS::GfVec4f(
                static_cast<float>(im[0]),
                static_cast<float>(im[1]),
                static_cast<float>(im[2]),
                static_cast<float>(quats[i].GetReal()));
        }
        return true;
    }
    return false;
}

/*! \brief  Acquire a float vertex buffer of numSplats entries, creating it when needed.

    VP2 pairs the streams of a shader with its fields by type, dimension and the order the buffers
    are bound in, not by name, so the descriptors have no name. Worker thread: the buffer is
    committed later, on the main thread.
*/
float* _AcquireStream(
    std::unique_ptr<MHWRender::MVertexBuffer>& buffer,
    MHWRender::MGeometry::Semantic             semantic,
    unsigned int                               dimension,
    size_t                                     numSplats)
{
    if (!buffer) {
        const MHWRender::MVertexBufferDescriptor desc(
            "", semantic, MHWRender::MGeometry::kFloat, dimension);
        buffer.reset(new MHWRender::MVertexBuffer(desc));
    }
    return static_cast<float*>(buffer->acquire(static_cast<unsigned int>(numSplats), true));
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

/*! \brief  Destructor

    The view cache unregisters itself and releases the shader clone through a later commit (see
    GaussianSplatViewCache::OnRprimDestroyed).
*/
HdVP2GaussianSplat::~HdVP2GaussianSplat()
{
    _viewCache->OnRprimDestroyed(_delegate->GetVP2ResourceRegistry());
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

    // The sorting mode hint has no dirty bit of its own: a change is seen at the next Sync caused
    // by something else, so it is read every time.
    _splatSharedData._sortingModeHint
        = delegate->Get(GetId(), PXR_NS::UsdVolTokens->sortingModeHint)
              .GetWithDefault<PXR_NS::TfToken>();

    // All splat attributes (positions, shape, opacity, SH coefficients) are primvars.
    const bool primvarDirty = (*dirtyBits & PXR_NS::HdChangeTracker::DirtyPrimvar) != 0;
    if (primvarDirty) {
        _ReadSplatData(delegate);
        _UpdateColorBuffer();
        _UpdateShapeBuffers();
    }

    _SyncEndCommon(*this, delegate, dirtyBits, reprToken, _sharedData, _reprs);

    // After _SyncEndCommon: it stores the local-to-world matrix that the sort and the view
    // dependent color use. Before it, the first Sync would see an identity matrix.
    _UpdateSplatData(primvarDirty);

    // Draw item update is controlled by its own dirty bits.
    _UpdateRepr(delegate, reprToken);
}

//! \brief  Read the splat attributes through the scene delegate. Missing attributes stay empty.
void HdVP2GaussianSplat::_ReadSplatData(PXR_NS::HdSceneDelegate* sceneDelegate)
{
    const PXR_NS::SdfPath&        id = GetId();
    HdVP2GaussianSplatSharedData& data = _splatSharedData;

    if (_GetVec3fArray(sceneDelegate->Get(id, PXR_NS::UsdVolTokens->positions), data._points)) {
        _CommitPositionsBuffer(data._positionsBuffer.get(), data._points);
    } else {
        TF_WARN("Gaussian splat %s has no positions.", id.GetText());
        data._points.clear();
    }

    // The plain names are requested: the scene delegate maps them to the half precision forms
    // when only those are authored.
    data._scales.clear();
    _GetVec3fArray(sceneDelegate->Get(id, PXR_NS::UsdVolTokens->scales), data._scales);

    data._orientations.clear();
    _GetOrientationArray(
        sceneDelegate->Get(id, PXR_NS::UsdVolTokens->orientations), data._orientations);

    data._opacities.clear();
    _GetFloatArray(sceneDelegate->Get(id, PXR_NS::UsdVolTokens->opacities), data._opacities);

    data._shCoeffs.clear();
    _GetVec3fArray(
        sceneDelegate->Get(id, PXR_NS::UsdVolTokens->radianceSphericalHarmonicsCoefficients),
        data._shCoeffs);

    // The schema default applies when the degree is not authored.
    data._shDegree = sceneDelegate->Get(id, PXR_NS::UsdVolTokens->radianceSphericalHarmonicsDegree)
                         .GetWithDefault<int>(kDefaultShDegree);

    // The color space is metadata of the coefficients attribute, not a primvar value: it is read
    // from the USD prim, as basisCurves.cpp does. It is re-read whenever the primvars are, but
    // the shader of a live Rprim is not rebuilt for it (see CreateSplatShaderClone).
    data._colorSpace.clear();
    auto* const param = static_cast<PXR_NS::HdVP2RenderParam*>(_delegate->GetRenderParam());
    const PXR_NS::UsdPrim usdPrim = param->GetDrawScene().GetUsdPrim(id);
    if (usdPrim) {
        const PXR_NS::UsdAttribute attr
            = usdPrim.GetAttribute(PXR_NS::UsdVolTokens->radianceSphericalHarmonicsCoefficients);
        if (attr) {
            data._colorSpace = attr.GetColorSpace().GetString();
        }
    }
}

/*! \brief  Size the color buffer, with a placeholder color (zeros) for every splat.

    The real color is view dependent (spherical harmonics evaluated for the viewing direction), so
    it is not known at sync time. It is computed from the full SH coefficients, which are not in
    any GPU buffer, only in the SplatData built by _UpdateSplatData:
      - GaussianSplatViewCore::ComputeColors evaluates it for each view (CPU), together with the
        back-to-front sort;
      - GaussianSplatViewCache::OnDraw (pre-draw callback, beauty pass only) then overwrites this
        buffer with the result, and the index buffer with the sorted order.

    Nothing reads this placeholder in the beauty pass: the callback always rewrites the buffer
    before the draw. That relies on the callback seeing the buffer as stale after this commit:
    every sync that reaches here also builds a new SplatData (see _UpdateSplatData, primvarDirty),
    and GaussianSplatViewCache::OnDataSynced forgets the buffered slot when it receives it. The
    other passes (shadow, depth) do not run the callback, so they see this placeholder.

    The opacity is not part of the color, it has its own stream, so alpha is zeroed too.
*/
void HdVP2GaussianSplat::_UpdateColorBuffer()
{
    const size_t numSplats = _splatSharedData._points.size();
    if (numSplats == 0) {
        return;
    }

    // The geometry shader reads this stream with the orientation stream. Both are float4, so
    // VP2 pairs them in the order they are bound in (see _UpdateDrawItem).
    float* bufferData = _AcquireStream(
        _splatSharedData._colorBuffer, MHWRender::MGeometry::kTexture, 4, numSplats);
    if (!bufferData) {
        return;
    }

    std::fill_n(bufferData, numSplats * 4, 0.0f);

    _CommitMVertexBuffer(_splatSharedData._colorBuffer.get(), bufferData);
}

/*! \brief  Build the immutable data handed to the view data (sort and view-dependent color).

    A new version is built when the splat attributes, the sorting mode hint or the local-to-world
    matrix changed. Each commit lambda publishes the latest one on the main thread.
*/
void HdVP2GaussianSplat::_UpdateSplatData(bool primvarDirty)
{
    using GaussianSplatViewCore::SplatData;

    const HdVP2GaussianSplatSharedData&     data = _splatSharedData;
    const PXR_NS::GfMatrix4d&               matrix = _sharedData.bounds.GetMatrix();
    const std::shared_ptr<const SplatData>& previous = _latestSplatData;

    if (!primvarDirty && previous && previous->sortingModeHint == data._sortingModeHint
        && previous->localToWorld == matrix) {
        return;
    }

    std::shared_ptr<SplatData> splat;
    if (primvarDirty || !previous) {
        splat = std::make_shared<SplatData>();
        const size_t numSplats = data._points.size();
        splat->points = data._points; // shares the array (copy-on-write)

        // shStride coefficients per splat, the rule ComputeColors applies too.
        const size_t shStride = numSplats ? data._shCoeffs.size() / numSplats : 0;
        if (shStride > 0 && data._shCoeffs.size() == shStride * numSplats) {
            splat->shCoefficients = data._shCoeffs; // shared, not copied
            splat->shStride = shStride;
        } else if (numSplats > 0) {
            // ComputeColors uses SplatData::defaultColor for every splat in this case.
            TF_WARN(
                "Gaussian splat %s: missing or mismatched SH coefficients (%zu for %zu splats), "
                "using default color.",
                GetId().GetText(),
                data._shCoeffs.size(),
                numSplats);
        }
        splat->shDegree = data._shDegree;

        auto* const param = static_cast<PXR_NS::HdVP2RenderParam*>(_delegate->GetRenderParam());
        splat->defaultColor
            = param->GetDrawScene().GetDefaultColor(PXR_NS::HdPrimTypeTokens->points);
    } else {
        // Matrix or hint change only: the points and SH arrays stay shared with the previous one.
        splat = std::make_shared<SplatData>(*previous);
    }
    splat->sortingModeHint = data._sortingModeHint;
    splat->localToWorld = matrix;
    _latestSplatData = std::move(splat);
}

/*! \brief  Fill the scale, orientation and opacity streams of the geometry shader.

    Each stream has one entry per splat. An array that is too long is truncated and one that is too
    short is ignored (USD schema rule). A missing stream is filled with the default value (unit
    scale, identity orientation, full opacity). It is not an error.
*/
void HdVP2GaussianSplat::_UpdateShapeBuffers()
{
    HdVP2GaussianSplatSharedData& data = _splatSharedData;
    const size_t                  numSplats = data._points.size();
    if (numSplats == 0) {
        return;
    }

    constexpr auto kTexture = MHWRender::MGeometry::kTexture;

    if (float* out = _AcquireStream(data._scaleBuffer, kTexture, 3, numSplats)) {
        const bool usable = data._scales.size() >= numSplats;
        for (size_t i = 0; i < numSplats; ++i) {
            for (size_t c = 0; c < 3; ++c) {
                out[i * 3 + c] = usable ? data._scales[i][c] : kDefaultScale;
            }
        }
        _CommitMVertexBuffer(data._scaleBuffer.get(), out);
    }

    if (float* out = _AcquireStream(data._orientationBuffer, kTexture, 4, numSplats)) {
        const bool usable = data._orientations.size() >= numSplats;
        for (size_t i = 0; i < numSplats; ++i) {
            const PXR_NS::GfVec4f q = usable ? data._orientations[i] : PXR_NS::GfVec4f(0, 0, 0, 1);
            std::copy_n(q.data(), 4, out + i * 4);
        }
        _CommitMVertexBuffer(data._orientationBuffer.get(), out);
    }

    if (float* out = _AcquireStream(data._opacityBuffer, kTexture, 1, numSplats)) {
        const bool usable = data._opacities.size() >= numSplats;
        for (size_t i = 0; i < numSplats; ++i) {
            out[i] = usable ? PXR_NS::GfClamp(data._opacities[i], 0.0f, 1.0f) : kDefaultOpacity;
        }
        _CommitMVertexBuffer(data._opacityBuffer.get(), out);
    }
}

/*! \brief  Returns the names of built-in primvars, i.e. primvars that are part of the core
            geometric schema for this prim.
*/
const PXR_NS::TfTokenVector& HdVP2GaussianSplat::GetBuiltinPrimvarNames() const
{
    static const PXR_NS::TfTokenVector primvarNames
        = { PXR_NS::UsdVolTokens->positions,
            PXR_NS::UsdVolTokens->scales,
            PXR_NS::UsdVolTokens->orientations,
            PXR_NS::UsdVolTokens->opacities,
            PXR_NS::UsdVolTokens->radianceSphericalHarmonicsCoefficients,
            PXR_NS::UsdVolTokens->radianceSphericalHarmonicsDegree,
            PXR_NS::UsdVolTokens->sortingModeHint };
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

    const MString&          name = drawItem->GetDrawItemName();
    MHWRender::MRenderItem* renderItem = nullptr;

    // The token-to-item mapping (Design 7). Only the items that draw splats carry the Rprim
    // shader (set by the first commit); the others never reach the registry.
    if (reprToken == PXR_NS::HdVP2ReprTokens->smoothHull) {
        renderItem = _CreateSplatRenderItem(name, MHWRender::MGeometry::kTextured);
#ifdef HAS_DEFAULT_MATERIAL_SUPPORT_API
        // As points.cpp: the shaded items are skipped when the default material is active.
        renderItem->setDefaultMaterialHandling(
            MHWRender::MRenderItem::SkipWhenDefaultMaterialActive);
#endif
    } else if (reprToken == PXR_NS::HdVP2ReprTokens->smoothHullUntextured) {
        renderItem = _CreateSplatRenderItem(name, MHWRender::MGeometry::kShaded);
#ifdef HAS_DEFAULT_MATERIAL_SUPPORT_API
        renderItem->setDefaultMaterialHandling(
            MHWRender::MRenderItem::SkipWhenDefaultMaterialActive);
#endif
    } else if (reprToken == PXR_NS::HdVP2ReprTokens->defaultMaterial) {
        renderItem = _CreateSplatRenderItem(
            name,
            static_cast<MHWRender::MGeometry::DrawMode>(
                MHWRender::MGeometry::kShaded | MHWRender::MGeometry::kTextured));
#ifdef HAS_DEFAULT_MATERIAL_SUPPORT_API
        // As mesh.cpp and basisCurves.cpp: the default material item draws only when the default
        // material is active. Skip here would draw a second set in every other style.
        renderItem->setDefaultMaterialHandling(
            MHWRender::MRenderItem::DrawOnlyWhenDefaultMaterialActive);
        // Keep the splat shader: Maya would swap in its default material shader, and neither the
        // splat stages nor the pre-draw callback would run (as Maya's own splat evaluator does).
        renderItem->setExcludedFromDefaultMaterialOverride(true);
#endif
    } else if (reprToken == PXR_NS::HdVP2ReprTokens->bbox) {
        // Box item of the authored extent, drawn in bounding box display style.
        renderItem = _CreateBoundingBoxRenderItem(
            name,
            kOpaqueBlue,
            MSelectionMask(MSelectionMask::kSelectParticleShapes),
            MHWRender::MFrameContext::kExcludeNParticles);
    } else if (reprToken == PXR_NS::HdReprTokens->wire) {
        // Same box item, drawn in wireframe display style.
        renderItem = _CreateBoundingBoxRenderItem(
            name,
            kOpaqueBlue,
            MSelectionMask(MSelectionMask::kSelectParticleShapes),
            MHWRender::MFrameContext::kExcludeNParticles);
        renderItem->setDrawMode(MHWRender::MGeometry::kWireframe);
    } else if (
        reprToken == PXR_NS::HdVP2ReprTokens->forcedBbox
        || reprToken == PXR_NS::HdVP2ReprTokens->forcedWire) {
        // Forced items serve instanced display-layer overrides, which splats do not support.
        // They are created disabled and never enabled (mesh.cpp does the same, then enables).
        renderItem = _CreateBoundingBoxRenderItem(
            name,
            kOpaqueBlue,
            MSelectionMask(MSelectionMask::kSelectParticleShapes),
            MHWRender::MFrameContext::kExcludeNParticles);
        renderItem->enable(false);
        if (reprToken == PXR_NS::HdVP2ReprTokens->forcedBbox) {
            renderItem->setDrawMode(MHWRender::MGeometry::kAll);
        } else {
            constexpr int sDrawModeAllButBBox
                = MHWRender::MGeometry::kAll & ~MHWRender::MGeometry::kBoundingBox;
            renderItem->setDrawMode(
                static_cast<MHWRender::MGeometry::DrawMode>(sDrawModeAllButBBox));
        }
    } else if (reprToken == PXR_NS::HdVP2ReprTokens->forcedUntextured) {
        renderItem = _CreateSplatRenderItem(name, MHWRender::MGeometry::kShaded);
        renderItem->enable(false);
    }

    if (renderItem) {
        drawItem->AddUsage(PXR_NS::HdVP2DrawItem::kSelectionHighlight);
        _AddRenderItem(*drawItem, renderItem, *subSceneContainer);
    }

#if HD_API_VERSION < 35
    repr->AddDrawItem(drawItem);
#else
    repr->AddDrawItem(std::move(drawItem));
#endif
}

//! \brief  What the given repr token draws.
HdVP2GaussianSplat::ItemKind HdVP2GaussianSplat::_GetItemKind(const PXR_NS::TfToken& reprToken)
{
    if (reprToken == PXR_NS::HdVP2ReprTokens->smoothHull
        || reprToken == PXR_NS::HdVP2ReprTokens->smoothHullUntextured
        || reprToken == PXR_NS::HdVP2ReprTokens->defaultMaterial) {
        return ItemKind::Splat;
    }
    if (reprToken == PXR_NS::HdVP2ReprTokens->bbox || reprToken == PXR_NS::HdReprTokens->wire) {
        return ItemKind::BBox;
    }
    if (reprToken == PXR_NS::HdVP2ReprTokens->forcedBbox
        || reprToken == PXR_NS::HdVP2ReprTokens->forcedWire
        || reprToken == PXR_NS::HdVP2ReprTokens->forcedUntextured) {
        return ItemKind::Disabled;
    }
    return ItemKind::None;
}

//! \brief  Create the render item used to draw the splats, visible in the given draw modes.
MHWRender::MRenderItem* HdVP2GaussianSplat::_CreateSplatRenderItem(
    const MString&                 name,
    MHWRender::MGeometry::DrawMode drawMode) const
{
    MHWRender::MRenderItem* const renderItem = MHWRender::MRenderItem::Create(
        name, MHWRender::MRenderItem::MaterialSceneItem, MHWRender::MGeometry::kPoints);

    renderItem->setDrawMode(drawMode);
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

    const ItemKind kind = _GetItemKind(reprToken);
    const size_t   numDrawItems = repr->GetDrawItems().size();
    for (size_t i = 0; i < numDrawItems; ++i) {
        PXR_NS::HdVP2DrawItem* drawItem = static_cast<PXR_NS::HdVP2DrawItem*>(repr->GetDrawItem(i));
        if (!drawItem) {
            continue;
        }
        if (kind == ItemKind::Splat) {
            _UpdateDrawItem(sceneDelegate, drawItem);
        } else if (kind == ItemKind::BBox) {
            _UpdateBBoxDrawItem(drawItem);
        } else {
            // Forced items stay disabled: they are never synced, and carry no geometry.
            drawItem->ResetDirtyBits();
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

    // The tint: selected splats and splats in template mode mix their color with the highlight
    // color (the mix strength is a constant of the shader). Read here, applied by the commit.
    const bool updateTint = (itemDirtyBits & DirtySelectionHighlight) != 0;
    bool       tintEnabled = false;
    float      tintColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    if (updateTint) {
        tintEnabled = _selectionStatus != kUnselected || inTemplateMode;
        if (tintEnabled) {
            const MColor color = _GetHighlightColor(PXR_NS::HdPrimTypeTokens->points);
            tintColor[0] = color.r;
            tintColor[1] = color.g;
            tintColor[2] = color.b;
            tintColor[3] = color.a < kOpaqueAlphaThreshold ? color.a : 1.0f;
        }
    }

    // Reset dirty bits because we've prepared commit state for this draw item.
    drawItem->ResetDirtyBits();

    MHWRender::MVertexBuffer* positionsBuffer = _splatSharedData._positionsBuffer.get();
    MHWRender::MVertexBuffer* colorBuffer = _splatSharedData._colorBuffer.get();
    MHWRender::MVertexBuffer* scaleBuffer = _splatSharedData._scaleBuffer.get();
    MHWRender::MVertexBuffer* orientationBuffer = _splatSharedData._orientationBuffer.get();
    MHWRender::MVertexBuffer* opacityBuffer = _splatSharedData._opacityBuffer.get();
    MHWRender::MIndexBuffer*  indexBuffer = drawItemData._indexBuffer.get();
    const std::shared_ptr<GaussianSplatViewCache>                 viewCache = _viewCache;
    const std::shared_ptr<const GaussianSplatViewCore::SplatData> splatData = _latestSplatData;
    const unsigned int numSplats = static_cast<unsigned int>(_splatSharedData._points.size());
    const std::string  colorSpace = _splatSharedData._colorSpace;

    _delegate->GetVP2ResourceRegistry().EnqueueCommit([drawItem,
                                                       stateToCommit,
                                                       param,
                                                       positionsBuffer,
                                                       colorBuffer,
                                                       scaleBuffer,
                                                       orientationBuffer,
                                                       opacityBuffer,
                                                       indexBuffer,
                                                       viewCache,
                                                       splatData,
                                                       numSplats,
                                                       colorSpace,
                                                       updateTint,
                                                       tintEnabled,
                                                       tintColor]() {
        // The Rprim is gone: its draw item is too, and the shader is released by the destructor.
        if (!viewCache->IsAlive()) {
            return;
        }

        MHWRender::MRenderItem* renderItem = drawItem->GetRenderItem();
        if (ARCH_UNLIKELY(!renderItem)) {
            return;
        }

        // The Rprim shader is a clone of the splat shader, created here (first commit, main
        // thread) and shared by every item of the Rprim.
        MHWRender::MShaderInstance* const shader = viewCache->EnsureShader(colorSpace);

        // Publish the data of the Rprim to the view cache, on the main thread. The draw only
        // reads it, and it runs during a draw, never during a commit.
        viewCache->OnDataSynced(splatData);

        if (shader) {
            // The tint uniforms of the clone (main thread, never from the callback).
            if (updateTint) {
                viewCache->SetTint(tintEnabled, tintColor);
            }
            if (renderItem->getShader() != shader) {
                renderItem->setShader(shader);
            }
        }
        // he shader outputs per-pixel transparency, which Maya can't detect statically. Flag the
        // item so it is drawn in the transparent pass with blending.
        renderItem->setTreatAsTransparent(true);

        if (stateToCommit._enabled != nullptr) {
            renderItem->enable(*stateToCommit._enabled);
        }

        PXR_NS::ProxyRenderDelegate& drawScene = param->GetDrawScene();

        // Set when this commit sizes the index buffer again (identity order).
        bool indexResized = false;

        if (numSplats > 0 && (stateToCommit._geometryDirty || stateToCommit._boundingBox)) {
            // The geometry shader pairs the streams with its fields by type, dimension and
            // the order they are bound in, not by name. The two float4 streams are paired in
            // the alphabetical order of their semantic names (splatColor before
            // splatOrientation), so the order is fixed: position, scale, color, orientation,
            // opacity. A failed bind silently shifts the pairing, so it is reported.
            MHWRender::MVertexBufferArray vertexBuffers;
            vertexBuffers.addBuffer(kPositionsStr, positionsBuffer);

            // The index buffer has one entry per splat, in drawing order. It is sized here, once
            // per count (identity order until the callback writes the sorted one). The callback
            // only rewrites it at this size.
            if (indexBuffer->size() != numSplats) {
                if (auto* const indices
                    = static_cast<uint32_t*>(indexBuffer->acquire(numSplats, true))) {
                    std::iota(indices, indices + numSplats, 0u);
                    indexBuffer->commit(indices);
                    // The shared buffers no longer hold a slot.
                    indexResized = true;
                }
            }

            if (scaleBuffer && colorBuffer && orientationBuffer && opacityBuffer) {
                bool added = true;
                added
                    &= vertexBuffers.addBuffer(kSplatScaleStream, scaleBuffer) == MStatus::kSuccess;
                added
                    &= vertexBuffers.addBuffer(kSplatColorStream, colorBuffer) == MStatus::kSuccess;
                added &= vertexBuffers.addBuffer(kSplatOrientationStream, orientationBuffer)
                    == MStatus::kSuccess;
                added &= vertexBuffers.addBuffer(kSplatOpacityStream, opacityBuffer)
                    == MStatus::kSuccess;
                if (!added) {
                    TF_WARN("Gaussian splat streams could not be bound.");
                }
            }

            drawScene.setGeometryForRenderItem(
                *renderItem, vertexBuffers, *indexBuffer, stateToCommit._boundingBox);
        }

        if (stateToCommit._worldMatrix != nullptr) {
            renderItem->setMatrix(stateToCommit._worldMatrix);
        }

        if (shader) {
            viewCache->OnBuffersCommitted(colorBuffer, indexBuffer, indexResized);
        }
    });
}

/*! \brief  Update the bounding box draw item (bbox and wire styles).

    The box is the authored extent: the shared unit cube is scaled and moved by the world matrix
    that _UpdateTransform computes from the extent. No extent is computed from the positions. The
    item is disabled when the extent is empty (_UpdateTransform keeps the stale matrix then) or
    when the prim has no splats.
*/
void HdVP2GaussianSplat::_UpdateBBoxDrawItem(PXR_NS::HdVP2DrawItem* drawItem)
{
    if (drawItem->GetRenderItems().empty()) {
        return;
    }

    PXR_NS::HdDirtyBits itemDirtyBits = drawItem->GetDirtyBits();

    PXR_NS::MayaUsdCommitState             stateToCommit(drawItem->GetRenderItemData());
    PXR_NS::HdVP2DrawItem::RenderItemData& drawItemData = stateToCommit._renderItemData;

    auto* const param = static_cast<PXR_NS::HdVP2RenderParam*>(_delegate->GetRenderParam());
    PXR_NS::ProxyRenderDelegate& drawScene = param->GetDrawScene();

    _UpdateTransform(stateToCommit, _sharedData, itemDirtyBits, /*isBoundingBoxItem*/ true);

    if (itemDirtyBits
        & (PXR_NS::HdChangeTracker::DirtyVisibility | PXR_NS::HdChangeTracker::DirtyRenderTag
           | PXR_NS::HdChangeTracker::DirtyPoints | PXR_NS::HdChangeTracker::DirtyPrimvar
           | PXR_NS::HdChangeTracker::DirtyExtent | DirtySelectionHighlight)) {
        bool enable = drawItem->GetVisible() && !_splatSharedData._points.empty()
            && !_sharedData.bounds.GetRange().IsEmpty();
        enable = enable && drawScene.DrawRenderTag(_splatSharedData._renderTag);

        if (drawItemData._enabled != enable) {
            drawItemData._enabled = enable;
            stateToCommit._enabled = &drawItemData._enabled;
        }
    }

    // The geometry is the shared unit cube, set once and when the bounds are first committed.
    stateToCommit._geometryDirty
        = (itemDirtyBits
           & (PXR_NS::HdChangeTracker::DirtyPoints | PXR_NS::HdChangeTracker::DirtyExtent
              | PXR_NS::HdChangeTracker::DirtyPrimvar));

    drawItem->ResetDirtyBits();

    if (stateToCommit.Empty()) {
        return;
    }

    const PXR_NS::HdVP2BBoxGeom&                  sharedBBoxGeom = _delegate->GetSharedBBoxGeom();
    const std::shared_ptr<GaussianSplatViewCache> viewCache = _viewCache;

    _delegate->GetVP2ResourceRegistry().EnqueueCommit(
        [drawItem, stateToCommit, param, &sharedBBoxGeom, viewCache]() {
            // The Rprim is gone, and its draw item with it.
            if (!viewCache->IsAlive()) {
                return;
            }

            MHWRender::MRenderItem* renderItem = drawItem->GetRenderItem();
            if (ARCH_UNLIKELY(!renderItem)) {
                return;
            }

            if (stateToCommit._enabled != nullptr) {
                renderItem->enable(*stateToCommit._enabled);
            }

            if (stateToCommit._geometryDirty || stateToCommit._boundingBox) {
                MHWRender::MVertexBufferArray vertexBuffers;
                vertexBuffers.addBuffer(
                    kPositionsStr,
                    const_cast<MHWRender::MVertexBuffer*>(sharedBBoxGeom.GetPositionBuffer()));
                param->GetDrawScene().setGeometryForRenderItem(
                    *renderItem,
                    vertexBuffers,
                    *sharedBBoxGeom.GetIndexBuffer(),
                    stateToCommit._boundingBox);
            }

            if (stateToCommit._worldMatrix != nullptr) {
                renderItem->setMatrix(stateToCommit._worldMatrix);
            }
        });
}

} // namespace MAYAUSD_NS_DEF
