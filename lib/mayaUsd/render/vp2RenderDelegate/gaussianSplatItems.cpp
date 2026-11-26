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
// Render items of HdVP2GaussianSplat: one item per repr token, chosen in _InitRepr.
//
#include "gaussianSplatItems.h"

#if PXR_VERSION >= 2605

#include "drawItem.h"
#include "gaussianSplat.h"
#include "renderDelegate.h"
#include "tokens.h"

#include <pxr/base/arch/hints.h>
#include <pxr/imaging/hd/repr.h>
#include <pxr/imaging/hd/tokens.h>

#include <maya/MSelectionMask.h>

namespace MAYAUSD_NS_DEF {

bool HdVP2GaussianSplatGetItemDesc(
    const PXR_NS::TfToken&      reprToken,
    HdVP2GaussianSplatItemDesc& desc)
{
    desc = HdVP2GaussianSplatItemDesc();
    if (reprToken == PXR_NS::HdVP2ReprTokens->smoothHull) {
        desc._drawMode = MHWRender::MGeometry::kTextured;
        return true;
    }
    if (reprToken == PXR_NS::HdVP2ReprTokens->smoothHullUntextured) {
        desc._drawMode = MHWRender::MGeometry::kShaded;
        return true;
    }
    if (reprToken == PXR_NS::HdVP2ReprTokens->defaultMaterial) {
#if MAYA_API_VERSION >= 20250000
        // The item keeps the splat shader in default material mode, see _CreateSplatRenderItem.
        desc._drawMode = static_cast<MHWRender::MGeometry::DrawMode>(
            MHWRender::MGeometry::kShaded | MHWRender::MGeometry::kTextured);
        desc._drawOnlyInDefaultMaterialMode = true;
        return true;
#else
        // Without the exclusion API, the splat prim draws nothing in default material mode.
        return false;
#endif
    }
    if (reprToken == PXR_NS::HdReprTokens->wire) {
        desc._isWire = true;
        desc._drawMode = MHWRender::MGeometry::kWireframe;
        return true;
    }
    // No item for the bounding box and the forced reprs (a bounding box layer hides splats).
    return false;
}

/*! \brief  Initialize the given representation of this Rprim.

    The repr is always created first, for every token, because Hydra raises a coding error when
    Sync finds no repr. Only the draw item and the render item depend on the token.
*/
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

    HdVP2GaussianSplatItemDesc desc;
    if (!HdVP2GaussianSplatGetItemDesc(reprToken, desc)) {
        return;
    }

#if HD_API_VERSION < 35
    auto* drawItem = new PXR_NS::HdVP2DrawItem(_delegate, &_sharedData);
#else
    std::unique_ptr<PXR_NS::HdVP2DrawItem> drawItem
        = std::make_unique<PXR_NS::HdVP2DrawItem>(_delegate, &_sharedData);
#endif

    MHWRender::MRenderItem* renderItem = _CreateSplatRenderItem(drawItem->GetDrawItemName(), desc);
    drawItem->AddUsage(PXR_NS::HdVP2DrawItem::kSelectionHighlight);
    _AddRenderItem(*drawItem, renderItem, *subSceneContainer);

#if HD_API_VERSION < 35
    repr->AddDrawItem(drawItem);
#else
    repr->AddDrawItem(std::move(drawItem));
#endif
}

/*! \brief  Create a render item of the splats, shaded or wire.

    All the splat items use the splat shader, in every mode. They are opaque, non-indexed (the
    primitive ID is the splat number) and never consolidated, because consolidation merges draws
    and offsets the primitive ID. The wire item draws a fixed-size quad per splat, in the
    wireframe draw mode.
*/
MHWRender::MRenderItem* HdVP2GaussianSplat::_CreateSplatRenderItem(
    const MString&                    name,
    const HdVP2GaussianSplatItemDesc& desc) const
{
    MHWRender::MRenderItem* const renderItem = MHWRender::MRenderItem::Create(
        name, MHWRender::MRenderItem::MaterialSceneItem, MHWRender::MGeometry::kPoints);

    renderItem->setDrawMode(desc._drawMode);
    renderItem->castsShadows(false);
    renderItem->receivesShadows(false);
    renderItem->setTreatAsTransparent(false);
    // The shared template, until the commit sets the Rprim's own clone.
    renderItem->setShader(_delegate->GetSplatFallbackShader(MColor()));
    _InitRenderItemCommon(renderItem);
    _SetWantConsolidation(*renderItem, false);

#if MAYA_API_VERSION >= 20250000
    // Keep the splat shader when Maya replaces the shaders in default material mode.
    renderItem->setExcludedFromDefaultMaterialOverride(true);
#endif

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
    MSelectionMask selectionMask(MSelectionMask::kSelectParticleShapes);
    selectionMask.addMask(MSelectionMask::kSelectPointsForGravity);
    renderItem->setSelectionMask(selectionMask);
#else
    renderItem->setSelectionMask(MSelectionMask::kSelectParticleShapes);
#endif

    renderItem->setObjectTypeExclusionFlag(MHWRender::MFrameContext::kExcludeNParticles);

#ifdef HAS_DEFAULT_MATERIAL_SUPPORT_API
    if (!desc._isWire) {
        renderItem->setDefaultMaterialHandling(
            desc._drawOnlyInDefaultMaterialMode
                ? MHWRender::MRenderItem::DrawOnlyWhenDefaultMaterialActive
                : MHWRender::MRenderItem::SkipWhenDefaultMaterialActive);
    }
#endif

    return renderItem;
}

} // namespace MAYAUSD_NS_DEF

#endif // PXR_VERSION >= 2605
