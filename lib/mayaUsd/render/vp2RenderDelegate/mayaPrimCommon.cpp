//
// Copyright 2021 Autodesk
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

#include "mayaPrimCommon.h"

#include "bboxGeom.h"
#include "material.h"
#include "render_delegate.h"
#include "tokens.h"

#ifdef MAYA_HAS_DISPLAY_LAYER_API
#include <mayaUsd/ufe/Utils.h>
#endif

#include <pxr/usdImaging/usdImaging/delegate.h>

#ifdef MAYA_HAS_DISPLAY_LAYER_API
#include <maya/MFnDisplayLayer.h>
#include <maya/MFnDisplayLayerManager.h>
#include <maya/MObjectArray.h>
#endif
#include <maya/MProfiler.h>
#ifdef MAYA_HAS_DISPLAY_LAYER_API
#include <ufe/pathString.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

const MColor MayaUsdRPrim::kOpaqueBlue(0.0f, 0.0f, 1.0f, 1.0f);
const MColor MayaUsdRPrim::kOpaqueGray(.18f, .18f, .18f, 1.0f);

const MString MayaUsdRPrim::kPositionsStr("positions");
const MString MayaUsdRPrim::kNormalsStr("normals");
const MString MayaUsdRPrim::kDiffuseColorStr("diffuseColor");
const MString MayaUsdRPrim::kSolidColorStr("solidColor");

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT

namespace {

std::mutex        sUfePathsMutex;
MayaUsdCustomData sMayaUsdCustomData;

constexpr auto sDrawModeAllButBBox = (MHWRender::MGeometry::DrawMode)(
    MHWRender::MGeometry::kAll & ~MHWRender::MGeometry::kBoundingBox);
} // namespace

/* static */
InstanceIdMap& MayaUsdCustomData::Get(const MHWRender::MRenderItem& renderItem)
{
    return sMayaUsdCustomData._itemData[renderItem.InternalObjectId()]._instanceIdMap;
}

/* static */
void MayaUsdCustomData::Remove(const MHWRender::MRenderItem& renderItem)
{
    // not thread safe, so if they are destroyed in parallel this will crash.
    // consider concurrent_hash_map for locking version that can erase
    sMayaUsdCustomData._itemData.unsafe_erase(renderItem.InternalObjectId());
}

/* static */
bool MayaUsdCustomData::ItemDataDirty(const MHWRender::MRenderItem& renderItem)
{
    // not thread safe, so if they are destroyed in parallel this will crash.
    // consider concurrent_hash_map for locking version that can erase
    return sMayaUsdCustomData._itemData[renderItem.InternalObjectId()]._itemDataDirty;
}

/* static */
void MayaUsdCustomData::ItemDataDirty(const MHWRender::MRenderItem& renderItem, bool dirty)
{
    // not thread safe, so if they are destroyed in parallel this will crash.
    // consider concurrent_hash_map for locking version that can erase
    sMayaUsdCustomData._itemData[renderItem.InternalObjectId()]._itemDataDirty = dirty;
}

/* static */
InstancePrimPaths& MayaUsdCustomData::GetInstancePrimPaths(const SdfPath& prim)
{
    return sMayaUsdCustomData._primData[prim]._instancePrimPaths;
}

/* static */
void MayaUsdCustomData::RemoveInstancePrimPaths(const SdfPath& prim)
{
    sMayaUsdCustomData._primData.unsafe_erase(prim);
}

#endif

MayaUsdRPrim::MayaUsdRPrim(HdVP2RenderDelegate* delegate, const SdfPath& id)
    : _delegate(delegate)
    , _rprimId(id.GetText())
{
    // Store a string version of the Cache Path to be used to tag MRenderItems. The CachePath is
    // equivalent to the USD segment of the items full Ufe::Path.
    auto* const          param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    ProxyRenderDelegate& drawScene = param->GetDrawScene();
    _PrimSegmentString.append(
        drawScene.GetScenePrimPath(id, UsdImagingDelegate::ALL_INSTANCES).GetString().c_str());
}

void MayaUsdRPrim::_CommitMVertexBuffer(MHWRender::MVertexBuffer* const buffer, void* bufferData)
    const
{
    const MString& rprimId = _rprimId;

    _delegate->GetVP2ResourceRegistry().EnqueueCommit(
        [buffer, bufferData, rprimId]() { buffer->commit(bufferData); });
}

void MayaUsdRPrim::_SetWantConsolidation(MHWRender::MRenderItem& renderItem, bool state)
{
#if MAYA_API_VERSION >= 20190000
    renderItem.setWantConsolidation(state);
#else
    renderItem.setWantSubSceneConsolidation(state);
#endif
}

void MayaUsdRPrim::_UpdateTransform(
    MayaUsdCommitState&      stateToCommit,
    const HdRprimSharedData& sharedData,
    const HdDirtyBits        itemDirtyBits,
    const bool               isBoundingBoxItem)
{
    HdVP2DrawItem::RenderItemData& drawItemData = stateToCommit._renderItemData;

    // Local bounds
    const GfRange3d& range = sharedData.bounds.GetRange();

    // Bounds are updated through MPxSubSceneOverride::setGeometryForRenderItem()
    // which is expensive, so it is updated only when it gets expanded in order
    // to reduce calling frequence.
    if (itemDirtyBits & HdChangeTracker::DirtyExtent) {
        const GfRange3d& rangeToUse
            = isBoundingBoxItem ? _delegate->GetSharedBBoxGeom().GetRange() : range;

        // If the Rprim has empty bounds, we will assign a null bounding box to the render item and
        // Maya will compute the bounding box from the position data.
        if (!rangeToUse.IsEmpty()) {
            const GfVec3d& min = rangeToUse.GetMin();
            const GfVec3d& max = rangeToUse.GetMax();

            bool boundingBoxExpanded = false;

            const MPoint pntMin(min[0], min[1], min[2]);
            if (!drawItemData._boundingBox.contains(pntMin)) {
                drawItemData._boundingBox.expand(pntMin);
                boundingBoxExpanded = true;
            }

            const MPoint pntMax(max[0], max[1], max[2]);
            if (!drawItemData._boundingBox.contains(pntMax)) {
                drawItemData._boundingBox.expand(pntMax);
                boundingBoxExpanded = true;
            }

            if (boundingBoxExpanded) {
                stateToCommit._boundingBox = &drawItemData._boundingBox;
            }
        }
    }

    // Local-to-world transformation
    MMatrix& worldMatrix = drawItemData._worldMatrix;
    sharedData.bounds.GetMatrix().Get(worldMatrix.matrix);

    // The bounding box draw item uses a globally-shared unit wire cube as the
    // geometry and transfers scale and offset of the bounds to world matrix.
    if (isBoundingBoxItem) {
        if ((itemDirtyBits & (HdChangeTracker::DirtyExtent | HdChangeTracker::DirtyTransform))
            && !range.IsEmpty()) {
            const GfVec3d midpoint = range.GetMidpoint();
            const GfVec3d size = range.GetSize();

            MPoint midp(midpoint[0], midpoint[1], midpoint[2]);
            midp *= worldMatrix;

            auto& m = worldMatrix.matrix;
            m[0][0] *= size[0];
            m[0][1] *= size[0];
            m[0][2] *= size[0];
            m[0][3] *= size[0];
            m[1][0] *= size[1];
            m[1][1] *= size[1];
            m[1][2] *= size[1];
            m[1][3] *= size[1];
            m[2][0] *= size[2];
            m[2][1] *= size[2];
            m[2][2] *= size[2];
            m[2][3] *= size[2];
            m[3][0] = midp[0];
            m[3][1] = midp[1];
            m[3][2] = midp[2];
            m[3][3] = midp[3];

            stateToCommit._worldMatrix = &drawItemData._worldMatrix;
        }
    } else if (itemDirtyBits & HdChangeTracker::DirtyTransform) {
        stateToCommit._worldMatrix = &drawItemData._worldMatrix;
    }
}

void MayaUsdRPrim::_FirstInitRepr(HdDirtyBits* dirtyBits, SdfPath const& id)
{
    auto* const param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());

    // Update selection state when it is a new Rprim. DirtySelectionHighlight
    // will be propagated to all draw items, to trigger sync for each repr.
    const HdVP2SelectionStatus selectionStatus = param->GetDrawScene().GetSelectionStatus(id);
    if (_selectionStatus != selectionStatus) {
        _selectionStatus = selectionStatus;
        *dirtyBits |= DirtySelectionHighlight;
    } else if (_selectionStatus == kPartiallySelected) {
        *dirtyBits |= DirtySelectionHighlight;
    }
}

void MayaUsdRPrim::_SetDirtyRepr(const HdReprSharedPtr& repr)
{
    const auto& items = repr->GetDrawItems();
#if HD_API_VERSION < 35
    for (HdDrawItem* item : items) {
        HdVP2DrawItem* drawItem = static_cast<HdVP2DrawItem*>(item);
#else
    for (const HdRepr::DrawItemUniquePtr& item : items) {
        HdVP2DrawItem* const drawItem = static_cast<HdVP2DrawItem*>(item.get());
#endif
        if (drawItem) {
            for (auto& renderItemData : drawItem->GetRenderItems()) {
                if (renderItemData.GetDirtyBits() & HdChangeTracker::AllDirty) {
                    // About to be drawn, but the Repr is dirty. Add DirtyRepr so we know in
                    // _PropagateDirtyBits that we need to propagate the dirty bits of this draw
                    // items to ensure proper Sync
                    renderItemData.SetDirtyBits(HdChangeTracker::DirtyRepr);
                }
            }
        }
    }
}

void DisableRenderItem(HdVP2DrawItem::RenderItemData& renderItemData, HdVP2RenderDelegate* delegate)
{
    renderItemData._enabled = false;
    delegate->GetVP2ResourceRegistry().EnqueueCommit(
        [&renderItemData]() { renderItemData._renderItem->enable(false); });
}

void MayaUsdRPrim::_UpdateReprOverrides(ReprVector& reprs)
{
    if (_reprOverride != _displayLayerModes._reprOverride) {
        _reprOverride = _displayLayerModes._reprOverride;

        RenderItemFunc updateItemsForReprOverride;
        if (_reprOverride == kBBox) {
            // In bbox mode, disable all representations except the bounding box representation,
            // which now will be visible in all the draw modes
            updateItemsForReprOverride = [this](HdVP2DrawItem::RenderItemData& renderItemData) {
                if (renderItemData._renderItem->drawMode() & MHWRender::MGeometry::kBoundingBox) {
                    renderItemData._renderItem->setDrawMode(MHWRender::MGeometry::kAll);
                } else {
                    DisableRenderItem(renderItemData, _delegate);
                }
            };
        } else if (_reprOverride == kWire) {
            // BBox representation is stronger than wire representation so it will not be affected
            // by unshaded mode. All other representations are disabled except the wireframe
            // representation, which now will be visible in all the other draw modes.
            updateItemsForReprOverride = [this](HdVP2DrawItem::RenderItemData& renderItemData) {
                if (renderItemData._renderItem->drawMode() & MHWRender::MGeometry::kBoundingBox) {
                    renderItemData._renderItem->setDrawMode(MHWRender::MGeometry::kBoundingBox);
                } else if (
                    renderItemData._renderItem->drawMode() & MHWRender::MGeometry::kWireframe) {
                    renderItemData._renderItem->setDrawMode(sDrawModeAllButBBox);
                } else {
                    DisableRenderItem(renderItemData, _delegate);
                }
            };
        } else {
            // If repr override is disabled, set bbox and wireframe representations back
            updateItemsForReprOverride = [this](HdVP2DrawItem::RenderItemData& renderItemData) {
                if (renderItemData._renderItem->drawMode() & MHWRender::MGeometry::kBoundingBox) {
                    renderItemData._renderItem->setDrawMode(MHWRender::MGeometry::kBoundingBox);
                } else if (
                    renderItemData._renderItem->drawMode() & MHWRender::MGeometry::kWireframe) {
                    renderItemData._renderItem->setDrawMode(MHWRender::MGeometry::kWireframe);
                }
            };
        }

        _ForEachRenderItem(reprs, updateItemsForReprOverride);
    }
}

TfToken MayaUsdRPrim::_GetOverrideToken(TfToken const& reprToken) const
{
    if (_reprOverride == kBBox) {
        return HdVP2ReprTokens->bbox;
    } else if (_reprOverride == kWire) {
        // BBox representation is strong than Wire representation, so it will not be overridden
        if (reprToken != HdVP2ReprTokens->bbox) {
            return HdReprTokens->wire;
        }
    }
    return TfToken();
}

HdReprSharedPtr MayaUsdRPrim::_InitReprCommon(
    HdRprim&       refThis,
    TfToken const& reprToken,
    ReprVector&    reprs,
    HdDirtyBits*   dirtyBits,
    SdfPath const& id)
{
    if (reprs.empty()) {
        _FirstInitRepr(dirtyBits, id);
    }

    _SyncDisplayLayerModes(refThis);

    _UpdateReprOverrides(reprs);

    // Finally we can get to actually init/dirty the repr
    ReprVector::const_iterator it
        = std::find_if(reprs.begin(), reprs.end(), [reprToken](ReprVector::const_reference e) {
              return reprToken == e.first;
          });
    HdReprSharedPtr curRepr = (it != reprs.end() ? it->second : nullptr);

    // In repr override mode, call InitRepr for the representation that overrides.
    if (_reprOverride != kNone) {
        TfToken overrideToken = _GetOverrideToken(reprToken);
        if (!overrideToken.IsEmpty() && (overrideToken != reprToken)) {
            auto* const param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
            auto&       drawScene = param->GetDrawScene();
            refThis.InitRepr(drawScene.GetUsdImagingDelegate(), overrideToken, dirtyBits);
            if (curRepr) {
                return nullptr; // if the overriden repr is already created, we can safely exit here
            }
        }
    }

    if (curRepr) {
        _SetDirtyRepr(curRepr);
        return nullptr;
    }

    // set dirty bit to say we need to sync a new repr
    *dirtyBits |= HdChangeTracker::NewRepr;

    reprs.emplace_back(reprToken, std::make_shared<HdRepr>());
    return reprs.back().second;
}

void MayaUsdRPrim::_PropagateDirtyBitsCommon(HdDirtyBits& bits, const ReprVector& reprs) const
{
    if (bits & HdChangeTracker::AllDirty) {
        // RPrim is dirty, propagate dirty bits to all draw items.
        for (const std::pair<TfToken, HdReprSharedPtr>& pair : reprs) {
            const HdReprSharedPtr& repr = pair.second;
            const auto&            items = repr->GetDrawItems();
#if HD_API_VERSION < 35
            for (HdDrawItem* item : items) {
                if (HdVP2DrawItem* drawItem = static_cast<HdVP2DrawItem*>(item)) {
#else
            for (const HdRepr::DrawItemUniquePtr& item : items) {
                if (HdVP2DrawItem* const drawItem = static_cast<HdVP2DrawItem*>(item.get())) {
#endif
                    for (auto& renderItemData : drawItem->GetRenderItems()) {
                        renderItemData.SetDirtyBits(bits);
                    }
                }
            }
        }
    } else {
        // RPrim is clean, find out if any drawItem about to be shown is dirty:
        for (const std::pair<TfToken, HdReprSharedPtr>& pair : reprs) {
            const HdReprSharedPtr& repr = pair.second;
            const auto&            items = repr->GetDrawItems();
#if HD_API_VERSION < 35
            for (const HdDrawItem* item : items) {
                if (const HdVP2DrawItem* drawItem = static_cast<const HdVP2DrawItem*>(item)) {
#else
            for (const HdRepr::DrawItemUniquePtr& item : items) {
                if (const HdVP2DrawItem* const drawItem = static_cast<HdVP2DrawItem*>(item.get())) {
#endif
                    // Is this Repr dirty and in need of a Sync?
                    for (auto& renderItemData : drawItem->GetRenderItems()) {
                        if (renderItemData.GetDirtyBits() & HdChangeTracker::DirtyRepr) {
                            bits |= (renderItemData.GetDirtyBits() & ~HdChangeTracker::DirtyRepr);
                        }
                    }
                }
            }
        }
    }
}

void MayaUsdRPrim::_InitRenderItemCommon(MHWRender::MRenderItem* renderItem) const
{
#ifdef MAYA_MRENDERITEM_UFE_IDENTIFIER_SUPPORT
    auto* const          param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    ProxyRenderDelegate& drawScene = param->GetDrawScene();

    // setUfeIdentifiers is not thread-safe, so enqueue the call here for later processing
    _delegate->GetVP2ResourceRegistry().EnqueueCommit([this, &drawScene, renderItem]() {
        drawScene.setUfeIdentifiers(*renderItem, _PrimSegmentString);
    });
#endif

    _SetWantConsolidation(*renderItem, true);

#ifdef MAYA_HAS_RENDER_ITEM_HIDE_ON_PLAYBACK_API
    renderItem->setHideOnPlayback(_hideOnPlayback);
#endif
}

HdVP2DrawItem::RenderItemData& MayaUsdRPrim::_AddRenderItem(
    HdVP2DrawItem&          drawItem,
    MHWRender::MRenderItem* renderItem,
    MSubSceneContainer&     subSceneContainer,
    const HdGeomSubset*     geomSubset) const
{
    _delegate->GetVP2ResourceRegistry().EnqueueCommit(
        [&subSceneContainer, renderItem]() { subSceneContainer.add(renderItem); });

    auto& renderItemData = drawItem.AddRenderItem(renderItem, geomSubset);

    // Representation override modes require a special setup
    if (_reprOverride == kBBox) {
        if (renderItem->drawMode() & MHWRender::MGeometry::kBoundingBox) {
            renderItem->setDrawMode(MHWRender::MGeometry::kAll);
        } else {
            DisableRenderItem(renderItemData, _delegate);
        }
    } else if (_reprOverride == kWire) {
        if (renderItem->drawMode() & MHWRender::MGeometry::kBoundingBox) {
            // BBox mode is stronger than Wire mode so nothing to change here
        } else if (renderItem->drawMode() & MHWRender::MGeometry::kWireframe) {
            renderItem->setDrawMode(sDrawModeAllButBBox);
        } else {
            DisableRenderItem(renderItemData, _delegate);
        }
    }

    return renderItemData;
}

/*! \brief  Create render item for bbox repr.
 */
MHWRender::MRenderItem* MayaUsdRPrim::_CreateBoundingBoxRenderItem(
    const MString&        name,
    const MColor&         color,
    const MSelectionMask& selectionMask,
    MUint64               exclusionFlag) const
{
    MHWRender::MRenderItem* const renderItem = MHWRender::MRenderItem::Create(
        name, MHWRender::MRenderItem::DecorationItem, MHWRender::MGeometry::kLines);

    renderItem->setDrawMode(MHWRender::MGeometry::kBoundingBox);
    renderItem->castsShadows(false);
    renderItem->receivesShadows(false);
    renderItem->setShader(_delegate->Get3dSolidShader(color));
    renderItem->setSelectionMask(selectionMask);
    _InitRenderItemCommon(renderItem);

#if MAYA_API_VERSION >= 20220000
    renderItem->setObjectTypeExclusionFlag(exclusionFlag);
#endif

    return renderItem;
}

/*! \brief  Create render item for wireframe repr.
 */
MHWRender::MRenderItem* MayaUsdRPrim::_CreateWireframeRenderItem(
    const MString&        name,
    const MColor&         color,
    const MSelectionMask& selectionMask,
    MUint64               exclusionFlag) const
{
    MHWRender::MRenderItem* const renderItem = MHWRender::MRenderItem::Create(
        name, MHWRender::MRenderItem::DecorationItem, MHWRender::MGeometry::kLines);

    renderItem->setDrawMode(MHWRender::MGeometry::kWireframe);
    renderItem->depthPriority(MHWRender::MRenderItem::sDormantWireDepthPriority);
    renderItem->castsShadows(false);
    renderItem->receivesShadows(false);
    renderItem->setShader(_delegate->Get3dSolidShader(color));

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
    MSelectionMask selectionMasks(selectionMask);
    selectionMasks.addMask(MSelectionMask::kSelectPointsForGravity);
    renderItem->setSelectionMask(selectionMasks);
#else
    renderItem->setSelectionMask(selectionMask);
#endif
    _InitRenderItemCommon(renderItem);

#if MAYA_API_VERSION >= 20220000
    renderItem->setObjectTypeExclusionFlag(exclusionFlag);
#endif

    return renderItem;
}

#ifndef MAYA_NEW_POINT_SNAPPING_SUPPORT
/*! \brief  Create render item for points repr.
 */
MHWRender::MRenderItem* MayaUsdRPrim::_CreatePointsRenderItem(
    const MString&        name,
    const MSelectionMask& selectionMask,
    MUint64               exclusionFlag) const
{
    MHWRender::MRenderItem* const renderItem = MHWRender::MRenderItem::Create(
        name, MHWRender::MRenderItem::DecorationItem, MHWRender::MGeometry::kPoints);

    renderItem->setDrawMode(MHWRender::MGeometry::kSelectionOnly);
    renderItem->depthPriority(MHWRender::MRenderItem::sDormantPointDepthPriority);
    renderItem->castsShadows(false);
    renderItem->receivesShadows(false);
    renderItem->setShader(_delegate->Get3dFatPointShader());

    MSelectionMask selectionMasks(selectionMask);
    selectionMasks.addMask(MSelectionMask::kSelectPointsForGravity);
    renderItem->setSelectionMask(selectionMasks);
    _InitRenderItemCommon(renderItem);

#if MAYA_API_VERSION >= 20220000
    renderItem->setObjectTypeExclusionFlag(exclusionFlag);
#endif

    return renderItem;
}
#endif

/*! \brief Hide all of the repr objects for this Rprim except the named repr.
    Repr objects are created to support specific reprName tokens, and contain a list of
    HdVP2DrawItems and corresponding RenderItems.
*/
void MayaUsdRPrim::_MakeOtherReprRenderItemsInvisible(
    const TfToken&    reprToken,
    const ReprVector& reprs)
{
    for (const std::pair<TfToken, HdReprSharedPtr>& pair : reprs) {
        if (pair.first != reprToken) {
            // For each relevant draw item, update dirty buffer sources.
            const HdReprSharedPtr& repr = pair.second;
            const auto&            items = repr->GetDrawItems();

#if HD_API_VERSION < 35
            for (HdDrawItem* item : items) {
                if (HdVP2DrawItem* drawItem = static_cast<HdVP2DrawItem*>(item)) {
#else
            for (const HdRepr::DrawItemUniquePtr& item : items) {
                if (HdVP2DrawItem* const drawItem = static_cast<HdVP2DrawItem*>(item.get())) {
#endif
                    for (auto& renderItemData : drawItem->GetRenderItems()) {
                        _delegate->GetVP2ResourceRegistry().EnqueueCommit([&renderItemData]() {
                            renderItemData._enabled = false;
                            renderItemData._renderItem->enable(false);
                        });
                    }
                }
            }
        }
    }
}

void MayaUsdRPrim::_HideAllDrawItems(HdReprSharedPtr const& curRepr)
{
    RenderItemFunc hideDrawItem = [this](HdVP2DrawItem::RenderItemData& renderItemData) {
        renderItemData._enabled = false;
        _delegate->GetVP2ResourceRegistry().EnqueueCommit(
            [&]() { renderItemData._renderItem->enable(false); });
    };

    _ForEachRenderItemInRepr(curRepr, hideDrawItem);
}

void MayaUsdRPrim::_ForEachRenderItemInRepr(const HdReprSharedPtr& curRepr, RenderItemFunc& func)
{
    if (!curRepr) {
        return;
    }

    const auto& items = curRepr->GetDrawItems();

#if HD_API_VERSION < 35
    for (HdDrawItem* item : items) {
        if (HdVP2DrawItem* drawItem = static_cast<HdVP2DrawItem*>(item)) {
#else
    for (const HdRepr::DrawItemUniquePtr& item : items) {
        if (HdVP2DrawItem* const drawItem = static_cast<HdVP2DrawItem*>(item.get())) {
#endif
            for (auto& renderItemData : drawItem->GetRenderItems()) {
                func(renderItemData);
            }
        }
    }
}

void MayaUsdRPrim::_ForEachRenderItem(const ReprVector& reprs, RenderItemFunc& func)
{
    for (const std::pair<TfToken, HdReprSharedPtr>& pair : reprs) {
        _ForEachRenderItemInRepr(pair.second, func);
    }
}

void MayaUsdRPrim::_UpdatePrimvarSourcesGeneric(
    HdSceneDelegate*       sceneDelegate,
    HdDirtyBits            dirtyBits,
    const TfTokenVector&   requiredPrimvars,
    HdRprim&               refThis,
    UpdatePrimvarInfoFunc& updatePrimvarInfo,
    ErasePrimvarInfoFunc&  erasePrimvarInfo)
{
    TfTokenVector::const_iterator begin = requiredPrimvars.cbegin();
    TfTokenVector::const_iterator end = requiredPrimvars.cend();

    // inspired by HdStInstancer::_SyncPrimvars
    // Get any required instanced primvars from the instancer. Get these before we get
    // any rprims from the rprim itself. If both are present, the rprim's values override
    // the instancer's value.
    const SdfPath& instancerId = refThis.GetInstancerId();
    if (!instancerId.IsEmpty()) {
        HdPrimvarDescriptorVector instancerPrimvars
            = sceneDelegate->GetPrimvarDescriptors(instancerId, HdInterpolationInstance);
        bool instancerDirty
            = ((dirtyBits
                & (HdChangeTracker::DirtyPrimvar | HdChangeTracker::DirtyInstancer
                   | HdChangeTracker::DirtyInstanceIndex))
               != 0);

        for (const HdPrimvarDescriptor& pv : instancerPrimvars) {
            if (std::find(begin, end, pv.name) == end) {
                // erase the unused primvar so we don't hold onto stale data
                erasePrimvarInfo(pv.name);
            } else {
                if (HdChangeTracker::IsPrimvarDirty(dirtyBits, instancerId, pv.name)
                    || instancerDirty) {
                    const VtValue value = sceneDelegate->Get(instancerId, pv.name);
                    updatePrimvarInfo(pv.name, value, HdInterpolationInstance);
                }
            }
        }
    }

    const SdfPath& id = refThis.GetId();
    for (size_t i = 0; i < HdInterpolationCount; i++) {
        const HdInterpolation           interp = static_cast<HdInterpolation>(i);
        const HdPrimvarDescriptorVector primvars
            = refThis.GetPrimvarDescriptors(sceneDelegate, interp);

        for (const HdPrimvarDescriptor& pv : primvars) {
            if (std::find(begin, end, pv.name) == end) {
                // erase the unused primvar so we don't hold onto stale data
                erasePrimvarInfo(pv.name);
            } else {
                if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, pv.name)) {
                    const VtValue value = refThis.GetPrimvar(sceneDelegate, pv.name);
                    updatePrimvarInfo(pv.name, value, interp);
                }
            }
        }
    }
}

void MayaUsdRPrim::_SyncDisplayLayerModes(const HdRprim&
#ifdef MAYA_HAS_DISPLAY_LAYER_API
                                              refThis
#endif
)
{
#ifdef MAYA_HAS_DISPLAY_LAYER_API
    auto* const          param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    ProxyRenderDelegate& drawScene = param->GetDrawScene();

    // First check if the status need updating
    if (drawScene.GetFrameCounter() == _displayLayerModesFrame) {
        return;
    }

    _displayLayerModesFrame = drawScene.GetFrameCounter();
    _displayLayerModes = DisplayLayerModes();

    // Display layer features are currently implemented only for non-instanced geometry
    if (refThis.GetInstancerId().IsEmpty()) {
        MFnDisplayLayerManager displayLayerManager(
            MFnDisplayLayerManager::currentDisplayLayerManager());
        MStatus status;
        MString pathString = drawScene.GetProxyShapeDagPath().fullPathName()
            + Ufe::PathString::pathSegmentSeparator().c_str() + _PrimSegmentString[0];

        MObjectArray ancestorDisplayLayers;
        {
            // Function getAncestorLayersInclusive is not multithreadable because of the
            // use of Ufe::Path inside, so we use a mutex here
            std::lock_guard<std::mutex> mutexGuard(sUfePathsMutex);
            ancestorDisplayLayers
                = displayLayerManager.getAncestorLayersInclusive(pathString, &status);
        }

        for (unsigned int i = 0; i < ancestorDisplayLayers.length(); i++) {
            MFnDependencyNode displayLayerNodeFn(ancestorDisplayLayers[i]);
            MPlug             layerEnabled = displayLayerNodeFn.findPlug("enabled");
            MPlug             layerVisible = displayLayerNodeFn.findPlug("visibility");
            MPlug             layerHidesOnPlayback = displayLayerNodeFn.findPlug("hideOnPlayback");
            MPlug             layerDisplayType = displayLayerNodeFn.findPlug("displayType");
            MPlug             levelOfDetail = displayLayerNodeFn.findPlug("levelOfDetail");
            MPlug             shading = displayLayerNodeFn.findPlug("shading");

            _displayLayerModes._visibility &= layerEnabled.asBool() ? layerVisible.asBool() : true;
            _displayLayerModes._hideOnPlayback |= layerHidesOnPlayback.asBool();
            if (levelOfDetail.asShort() != 0) {
                _displayLayerModes._reprOverride = kBBox;
            } else if (shading.asShort() == 0 && _displayLayerModes._reprOverride != kBBox) {
                _displayLayerModes._reprOverride = kWire;
            }
            if (_displayLayerModes._displayType == kNormal) {
                _displayLayerModes._displayType = (DisplayType)layerDisplayType.asShort();
            }
        }
    }
#endif
}

void MayaUsdRPrim::_SyncSharedData(
    HdRprimSharedData& sharedData,
    HdSceneDelegate*   delegate,
    HdDirtyBits const* dirtyBits,
    TfToken const&     reprToken,
    HdRprim const&     refThis,
    ReprVector const&  reprs,
    TfToken const&     renderTag)
{
    const SdfPath& id = refThis.GetId();

    if (HdChangeTracker::IsExtentDirty(*dirtyBits, id)) {
        sharedData.bounds.SetRange(delegate->GetExtent(id));
    }

    if (HdChangeTracker::IsTransformDirty(*dirtyBits, id)) {
        sharedData.bounds.SetMatrix(delegate->GetTransform(id));
    }

    if (HdChangeTracker::IsVisibilityDirty(*dirtyBits, id)) {
        bool usdVisibility = delegate->GetVisible(id);

        // Invisible rprims don't get calls to Sync or _PropagateDirtyBits while
        // they are invisible. This means that when a prim goes from visible to
        // invisible that we must update every repr, because if we switch reprs while
        // invisible we'll get no chance to update!
        if (!usdVisibility)
            _MakeOtherReprRenderItemsInvisible(reprToken, reprs);

        // Update "hide on playback" status
        if (_hideOnPlayback != _displayLayerModes._hideOnPlayback) {
            _hideOnPlayback = _displayLayerModes._hideOnPlayback;
#ifdef MAYA_HAS_RENDER_ITEM_HIDE_ON_PLAYBACK_API
            RenderItemFunc setHideOnPlayback
                = [this](HdVP2DrawItem::RenderItemData& renderItemData) {
                      renderItemData._renderItem->setHideOnPlayback(_hideOnPlayback);
                  };

            _ForEachRenderItem(reprs, setHideOnPlayback);
#endif
        }

        sharedData.visible = usdVisibility && _displayLayerModes._visibility;
    }

#if PXR_VERSION > 2111
    // Hydra now manages and caches render tags under the hood and is clearing
    // the dirty bit prior to calling sync. Unconditionally set the render tag
    // in the shared data structure based on current Hydra data
    _RenderTag() = renderTag;
#else
    if (*dirtyBits
        & (HdChangeTracker::DirtyRenderTag
#ifdef ENABLE_RENDERTAG_VISIBILITY_WORKAROUND
           | HdChangeTracker::DirtyVisibility
#endif
           )) {
        _RenderTag() = renderTag;
    }
#endif
}

bool MayaUsdRPrim::_SyncCommon(
    HdRprim&               refThis,
    HdSceneDelegate*       delegate,
    HdRenderParam*         renderParam,
    HdDirtyBits*           dirtyBits,
    HdReprSharedPtr const& curRepr,
    TfToken const&         reprToken)
{
    // In representation override mode call Sync for the representation override instead.
    if (_reprOverride != kNone) {
        TfToken overrideToken = _GetOverrideToken(reprToken);
        if (!overrideToken.IsEmpty() && (overrideToken != reprToken)) {
            refThis.Sync(delegate, renderParam, dirtyBits, overrideToken);
            return false;
        }
    }

    const SdfPath&       id = refThis.GetId();
    auto* const          param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    ProxyRenderDelegate& drawScene = param->GetDrawScene();

    // Update the selection status if it changed.
    if (*dirtyBits & DirtySelectionHighlight) {
        _selectionStatus = drawScene.GetSelectionStatus(id);
    } else {
        TF_VERIFY(_selectionStatus == drawScene.GetSelectionStatus(id));
    }

    // We don't update the repr if it is hidden by the render tags (purpose)
    // of the ProxyRenderDelegate. In additional, we need to hide any already
    // existing render items because they should not be drawn.
    HdRenderIndex& renderIndex = delegate->GetRenderIndex();
    if (!drawScene.DrawRenderTag(renderIndex.GetRenderTag(id))) {
        _HideAllDrawItems(curRepr);
        *dirtyBits &= ~(
            HdChangeTracker::DirtyRenderTag
#ifdef ENABLE_RENDERTAG_VISIBILITY_WORKAROUND
            | HdChangeTracker::DirtyVisibility
#endif
        );
        return false;
    }

    return true;
}

MColor MayaUsdRPrim::_GetHighlightColor(const TfToken& className)
{
    auto* const          param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    ProxyRenderDelegate& drawScene = param->GetDrawScene();

    if (_displayLayerModes._displayType == MayaUsdRPrim::kTemplate) {
        return drawScene.GetTemplateColor(_selectionStatus != kUnselected);
    } else if (
        _displayLayerModes._displayType == MayaUsdRPrim::kReference
        && _selectionStatus == kUnselected) {
        return drawScene.GetReferenceColor();
    } else {
        return (
            _selectionStatus != kUnselected ? drawScene.GetSelectionHighlightColor(
                _selectionStatus == kFullyLead ? TfToken() : className)
                                            : drawScene.GetWireframeColor());
    }
}

SdfPath MayaUsdRPrim::_GetUpdatedMaterialId(HdRprim* rprim, HdSceneDelegate* delegate)
{
    const SdfPath& id = rprim->GetId();
    const SdfPath  materialId = delegate->GetMaterialId(id);
    HdRenderIndex& renderIndex = delegate->GetRenderIndex();

    if (!materialId.IsEmpty()) {
        auto* material = dynamic_cast<HdVP2Material*>(
            renderIndex.GetSprim(HdPrimTypeTokens->material, materialId));
        if (material) {
            // Load the textures if any
            material->EnqueueLoadTextures();
        }
    }

#ifdef HDVP2_MATERIAL_CONSOLIDATION_UPDATE_WORKAROUND
    const SdfPath& origMaterialId = rprim->GetMaterialId();
    if (materialId != origMaterialId) {
        HdRenderIndex& renderIndex = delegate->GetRenderIndex();

        if (!origMaterialId.IsEmpty()) {
            HdVP2Material* material = static_cast<HdVP2Material*>(
                renderIndex.GetSprim(HdPrimTypeTokens->material, origMaterialId));
            if (material) {
                material->UnsubscribeFromMaterialUpdates(id);
            }
        }

        if (!materialId.IsEmpty()) {
            HdVP2Material* material = static_cast<HdVP2Material*>(
                renderIndex.GetSprim(HdPrimTypeTokens->material, materialId));
            if (material) {
                material->SubscribeForMaterialUpdates(id);
            }
        }
    }
#endif

    return materialId;
}

PXR_NAMESPACE_CLOSE_SCOPE
