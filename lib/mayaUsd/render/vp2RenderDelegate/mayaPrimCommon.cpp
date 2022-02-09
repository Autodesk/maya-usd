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
#include "render_delegate.h"

#include <pxr/usdImaging/usdImaging/delegate.h>

#include <maya/MProfiler.h>

PXR_NAMESPACE_OPEN_SCOPE

const MColor MayaUsdRPrim::kOpaqueBlue(0.0f, 0.0f, 1.0f, 1.0f);
const MColor MayaUsdRPrim::kOpaqueGray(.18f, .18f, .18f, 1.0f);

const MString MayaUsdRPrim::kPositionsStr("positions");
const MString MayaUsdRPrim::kNormalsStr("normals");
const MString MayaUsdRPrim::kDiffuseColorStr("diffuseColor");
const MString MayaUsdRPrim::kSolidColorStr("solidColor");

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT

namespace {

MayaUsdCustomData sMayaUsdCustomData;

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

    _delegate->GetVP2ResourceRegistry().EnqueueCommit([buffer, bufferData, rprimId]() {
        MProfilingScope profilingScope(
            HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorC_L2,
            "CommitBuffer",
            rprimId.asChar()); // TODO: buffer usage so we know it is positions normals etc

        buffer->commit(bufferData);
    });
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

/*! \brief  Create render item for bbox repr.
 */
MHWRender::MRenderItem* MayaUsdRPrim::_CreateBoundingBoxRenderItem(const MString& name, const MColor& color, const MSelectionMask& selectionMask, MUint64 exclusionFlag) const
{
    MHWRender::MRenderItem* const renderItem = MHWRender::MRenderItem::Create(
        name, MHWRender::MRenderItem::DecorationItem, MHWRender::MGeometry::kLines);

    renderItem->setDrawMode(MHWRender::MGeometry::kBoundingBox);
    renderItem->castsShadows(false);
    renderItem->receivesShadows(false);
    renderItem->setShader(_delegate->Get3dSolidShader(color));
    renderItem->setSelectionMask(selectionMask);
#ifdef MAYA_MRENDERITEM_UFE_IDENTIFIER_SUPPORT
    auto* const          param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    ProxyRenderDelegate& drawScene = param->GetDrawScene();
    drawScene.setUfeIdentifiers(*renderItem, _PrimSegmentString);
#endif

#if MAYA_API_VERSION >= 20220000
    renderItem->setObjectTypeExclusionFlag(exclusionFlag);
#endif

    _SetWantConsolidation(*renderItem, true);

    return renderItem;
}

/*! \brief  Create render item for wireframe repr.
 */
MHWRender::MRenderItem* MayaUsdRPrim::_CreateWireframeRenderItem(const MString& name, const MColor& color, const MSelectionMask& selectionMask, MUint64 exclusionFlag) const
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
#ifdef MAYA_MRENDERITEM_UFE_IDENTIFIER_SUPPORT
    auto* const          param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    ProxyRenderDelegate& drawScene = param->GetDrawScene();
    drawScene.setUfeIdentifiers(*renderItem, _PrimSegmentString);
#endif

#if MAYA_API_VERSION >= 20220000
    renderItem->setObjectTypeExclusionFlag(exclusionFlag);
#endif

    _SetWantConsolidation(*renderItem, true);

    return renderItem;
}

#ifndef MAYA_NEW_POINT_SNAPPING_SUPPORT
/*! \brief  Create render item for points repr.
 */
MHWRender::MRenderItem* MayaUsdRPrim::_CreatePointsRenderItem(const MString& name, const MSelectionMask& selectionMask, MUint64 exclusionFlag) const
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
#ifdef MAYA_MRENDERITEM_UFE_IDENTIFIER_SUPPORT
    auto* const          param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    ProxyRenderDelegate& drawScene = param->GetDrawScene();
    drawScene.setUfeIdentifiers(*renderItem, _PrimSegmentString);
#endif

#if MAYA_API_VERSION >= 20220000
    renderItem->setObjectTypeExclusionFlag(exclusionFlag);
#endif

    _SetWantConsolidation(*renderItem, true);

    return renderItem;
}
#endif

/*! \brief Hide all of the repr objects for this Rprim except the named repr.
    Repr objects are created to support specific reprName tokens, and contain a list of
    HdVP2DrawItems and corresponding RenderItems.
*/
void MayaUsdRPrim::_MakeOtherReprRenderItemsInvisible(
    const TfToken&   reprToken,
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
                renderItemData._enabled = false;
                _delegate->GetVP2ResourceRegistry().EnqueueCommit(
                    [&]() { renderItemData._renderItem->enable(false); });
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
