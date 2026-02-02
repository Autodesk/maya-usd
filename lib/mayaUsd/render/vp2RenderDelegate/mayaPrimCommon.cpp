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
#include "renderDelegate.h"
#include "tokens.h"

#include <pxr/usdImaging/usdImaging/delegate.h>

#ifdef MAYA_HAS_DISPLAY_LAYER_API
#include <mayaUsd/utils/util.h>

#include <maya/MFnDisplayLayer.h>
#include <maya/MFnDisplayLayerManager.h>
#include <maya/MObjectArray.h>
#endif
#include <maya/M3dView.h>
#include <maya/MProfiler.h>

PXR_NAMESPACE_OPEN_SCOPE

const MColor MayaUsdRPrim::kOpaqueBlue(0.0f, 0.0f, 1.0f, 1.0f);
const MColor MayaUsdRPrim::kOpaqueGray(.18f, .18f, .18f, 1.0f);

const MString MayaUsdRPrim::kPositionsStr("positions");
const MString MayaUsdRPrim::kNormalsStr("normals");
const MString MayaUsdRPrim::kDiffuseColorStr("diffuseColor");
const MString MayaUsdRPrim::kSolidColorStr("solidColor");

constexpr auto sDrawModeAllButBBox = (MHWRender::MGeometry::DrawMode)(
    MHWRender::MGeometry::kAll & ~MHWRender::MGeometry::kBoundingBox);

static const InstancePrototypePath sVoidInstancePrototypePath { SdfPath(), kNativeInstancing };

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT

namespace {
std::mutex        sMayaMutex;
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
    , _hydraId(id)
    , _rprimId(id.GetText())
{
    // Store a string version of the Cache Path to be used to tag MRenderItems. The CachePath is
    // equivalent to the USD segment of the items full Ufe::Path.
    auto* const          param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    ProxyRenderDelegate& drawScene = param->GetDrawScene();
    _PrimSegmentString.append(
        drawScene.GetScenePrimPath(id, UsdImagingDelegate::ALL_INSTANCES).GetString().c_str());
}

MayaUsdRPrim::~MayaUsdRPrim()
{
    if (!_pathInPrototype.first.IsEmpty()) {
        // Clear my entry from the instancing map
        auto* const          param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
        ProxyRenderDelegate& drawScene = param->GetDrawScene();
        drawScene.UpdateInstancingMapEntry(_pathInPrototype, sVoidInstancePrototypePath, _hydraId);
    }
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
    renderItem.setWantConsolidation(state);
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
    // The bounding box draw item uses a globally-shared unit wire cube as the
    // geometry and transfers scale and offset of the bounds to world matrix.
    if (isBoundingBoxItem) {
        if ((itemDirtyBits & (HdChangeTracker::DirtyExtent | HdChangeTracker::DirtyTransform))
            && !range.IsEmpty()) {
            sharedData.bounds.GetMatrix().Get(drawItemData._worldMatrix.matrix);

            const GfVec3d midpoint = range.GetMidpoint();
            const GfVec3d size = range.GetSize();

            MPoint midp(midpoint[0], midpoint[1], midpoint[2]);
            midp *= drawItemData._worldMatrix;

            auto& m = drawItemData._worldMatrix.matrix;
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
        sharedData.bounds.GetMatrix().Get(drawItemData._worldMatrix.matrix);
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
    RenderItemFunc setDirtyRepr = [](HdVP2DrawItem::RenderItemData& renderItemData) {
        if (renderItemData.GetDirtyBits() & HdChangeTracker::AllDirty) {
            // About to be drawn, but the Repr is dirty. Add DirtyRepr so we know in
            // _PropagateDirtyBits that we need to propagate the dirty bits of this draw
            // items to ensure proper Sync
            renderItemData.SetDirtyBits(HdChangeTracker::DirtyRepr);
        }
    };

    _ForEachRenderItemInRepr(repr, setDirtyRepr);
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
            updateItemsForReprOverride = [](HdVP2DrawItem::RenderItemData& renderItemData) {
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

TfToken MayaUsdRPrim::_GetMaterialNetworkToken(const TfToken& reprToken) const
{
    return _displayLayerModes._texturing ? reprToken : TfToken();
}

HdReprSharedPtr MayaUsdRPrim::_FindRepr(const ReprVector& reprs, const TfToken& reprToken)
{
    ReprVector::const_iterator it
        = std::find_if(reprs.begin(), reprs.end(), [reprToken](ReprVector::const_reference e) {
              return reprToken == e.first;
          });

    return (it != reprs.end() ? it->second : nullptr);
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

    auto* const param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    auto&       drawScene = param->GetDrawScene();

    // See if the primitive is instanced
    auto delegate = drawScene.GetUsdImagingDelegate();
    auto instancerId = delegate->GetInstancerId(id);
    bool instanced = !instancerId.IsEmpty();
    // The additional condition below is to prevent a crash in USD function GetScenePrimPath
    instanced = (instanced && !delegate->GetInstanceIndices(instancerId, id).empty());

    // display layers handling
    if (instanced) {
        // Sync display layer modes for instanced prims.
        // This also sets the value of '_useInstancedDisplayLayerModes' that identifies whether
        // display layer modes will be handled on per-primitive or per-instance basis
        _SyncDisplayLayerModes(id, true);

        // Instanced primitives with instances in display layers use 'forced' representations to
        // draw those specific instances, so the 'forced' representations should be inited alongside
        if (_useInstancedDisplayLayerModes && reprToken != HdVP2ReprTokens->forcedBbox
            && reprToken != HdVP2ReprTokens->forcedWire
            && reprToken != HdVP2ReprTokens->forcedUntextured) {
            refThis.InitRepr(
                drawScene.GetUsdImagingDelegate(), HdVP2ReprTokens->forcedBbox, dirtyBits);
            refThis.InitRepr(
                drawScene.GetUsdImagingDelegate(), HdVP2ReprTokens->forcedWire, dirtyBits);
            refThis.InitRepr(
                drawScene.GetUsdImagingDelegate(), HdVP2ReprTokens->forcedUntextured, dirtyBits);
        }
    } else {
        // Sync display layer modes for non-instanced prims.
        _SyncDisplayLayerModes(id, false);
    }

    _UpdateReprOverrides(reprs);

    // Find the current representation in the array of all inited representations
    HdReprSharedPtr curRepr = _FindRepr(reprs, reprToken);

    // In repr override mode, call InitRepr for the representation that overrides.
    if (_reprOverride != kNone) {
        TfToken overrideToken = _GetOverrideToken(reprToken);
        if (!overrideToken.IsEmpty() && (overrideToken != reprToken)) {
            refThis.InitRepr(drawScene.GetUsdImagingDelegate(), overrideToken, dirtyBits);
            if (curRepr) {
                return nullptr; // if the overriden repr is already created, we can safely exit here
            }
        }
    }

    // Finalize initialization

    if (curRepr) {
        _SetDirtyRepr(curRepr);
        return nullptr;
    }

    *dirtyBits |= HdChangeTracker::NewRepr; // set dirty bit to say we need to sync a new repr
    reprs.emplace_back(reprToken, std::make_shared<HdRepr>());
    return reprs.back().second;
}

void MayaUsdRPrim::_PropagateDirtyBitsCommon(HdDirtyBits& bits, const ReprVector& reprs) const
{
    if (bits & HdChangeTracker::AllDirty) {
        // RPrim is dirty, propagate dirty bits to all draw items.
        RenderItemFunc setDirtyBitsToItem = [bits](HdVP2DrawItem::RenderItemData& renderItemData) {
            renderItemData.SetDirtyBits(bits);
        };

        _ForEachRenderItem(reprs, setDirtyBitsToItem);
    } else {
        // RPrim is clean, find out if any drawItem about to be shown is dirty:
        RenderItemFunc setDirtyBitsFromItem
            = [&bits](HdVP2DrawItem::RenderItemData& renderItemData) {
                  if (renderItemData.GetDirtyBits() & HdChangeTracker::DirtyRepr) {
                      bits |= (renderItemData.GetDirtyBits() & ~HdChangeTracker::DirtyRepr);
                  }
              };

        _ForEachRenderItem(reprs, setDirtyBitsFromItem);
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

    renderItem->setObjectTypeExclusionFlag(exclusionFlag);

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

    renderItem->setObjectTypeExclusionFlag(exclusionFlag);

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

    renderItem->setObjectTypeExclusionFlag(exclusionFlag);

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
    RenderItemFunc disableRenderItem = [this](HdVP2DrawItem::RenderItemData& renderItemData) {
        _delegate->GetVP2ResourceRegistry().EnqueueCommit([&renderItemData]() {
            renderItemData._enabled = false;
            renderItemData._renderItem->enable(false);
        });
    };

    for (const std::pair<TfToken, HdReprSharedPtr>& pair : reprs) {
        if (pair.first != reprToken) {
            _ForEachRenderItemInRepr(pair.second, disableRenderItem);
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
        HdVP2DrawItem* drawItem = static_cast<HdVP2DrawItem*>(item);
#else
    for (const HdRepr::DrawItemUniquePtr& item : items) {
        HdVP2DrawItem* drawItem = static_cast<HdVP2DrawItem*>(item.get());
#endif
        for (; drawItem; drawItem = drawItem->GetMod()) {
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

#ifdef MAYA_HAS_DISPLAY_LAYER_API
void MayaUsdRPrim::_ProcessDisplayLayerModes(
    const MObject&     displayLayerObj,
    DisplayLayerModes& displayLayerModes)
{
    // Maya's MPlug API is not multithreadable, so we need the mutex here
    std::lock_guard<std::mutex> mutexGuard(sMayaMutex);

    MFnDependencyNode displayLayerNodeFn(displayLayerObj);
    MPlug             layerEnabled = displayLayerNodeFn.findPlug("enabled");
    if (!layerEnabled.asBool()) {
        return;
    }

    MPlug layerVisible = displayLayerNodeFn.findPlug("visibility");
    MPlug layerHidesOnPlayback = displayLayerNodeFn.findPlug("hideOnPlayback");
    MPlug layerDisplayType = displayLayerNodeFn.findPlug("displayType");
    MPlug levelOfDetail = displayLayerNodeFn.findPlug("levelOfDetail");
    MPlug shading = displayLayerNodeFn.findPlug("shading");
    MPlug texturing = displayLayerNodeFn.findPlug("texturing");
    MPlug colorIndex = displayLayerNodeFn.findPlug("color");
    MPlug useRGBColors = displayLayerNodeFn.findPlug("overrideRGBColors");
    MPlug colorRGB = displayLayerNodeFn.findPlug("overrideColorRGB");
    MPlug colorA = displayLayerNodeFn.findPlug("overrideColorA");

    displayLayerModes._visibility &= layerVisible.asBool();
    displayLayerModes._hideOnPlayback |= layerHidesOnPlayback.asBool();
    displayLayerModes._texturing = texturing.asBool();
    if (levelOfDetail.asShort() != 0) {
        displayLayerModes._reprOverride = kBBox;
    } else if (shading.asShort() == 0 && displayLayerModes._reprOverride != kBBox) {
        displayLayerModes._reprOverride = kWire;
    }
    if (displayLayerModes._displayType == kNormal) {
        displayLayerModes._displayType = (DisplayType)layerDisplayType.asShort();
    }

    if (useRGBColors.asBool()) {
        auto          colorRGBHolder = UsdMayaUtil::GetPlugDataHandle(colorRGB);
        const float3& rgbColor = colorRGBHolder->GetDataHandle().asFloat3();
        displayLayerModes._wireframeColorIndex = -1;
        displayLayerModes._wireframeColorRGBA
            = MColor(rgbColor[0], rgbColor[1], rgbColor[2], colorA.asFloat());
    } else {
        displayLayerModes._wireframeColorIndex = colorIndex.asInt();
    }
}

void MayaUsdRPrim::_PopulateDisplayLayerModes(
    const SdfPath&       usdPath,
    DisplayLayerModes&   displayLayerModes,
    ProxyRenderDelegate& drawScene)
{
    displayLayerModes = DisplayLayerModes();

    // First, process the hierarchy of usd paths
    auto ancestorsRange = usdPath.GetAncestorsRange();
    for (auto it = ancestorsRange.begin(); it != ancestorsRange.end(); ++it) {
        auto displayLayerObj = drawScene.GetDisplayLayer(*it);
        if (!displayLayerObj.isNull()) {
            _ProcessDisplayLayerModes(displayLayerObj, displayLayerModes);
        }
    }

    // Then, process the hierarchy inside Maya
    const MObjectArray& proxyShapeDisplayLayers = drawScene.GetProxyShapeDisplayLayers();
    auto                proxyShapeDisplayLayerCount = proxyShapeDisplayLayers.length();
    for (unsigned int j = 0; j < proxyShapeDisplayLayerCount; j++) {
        auto displayLayerObj = proxyShapeDisplayLayers[j];
        if (!displayLayerObj.isNull()) {
            _ProcessDisplayLayerModes(displayLayerObj, displayLayerModes);
        }
    }
}
#else
void MayaUsdRPrim::_PopulateDisplayLayerModes(
    const SdfPath&,
    DisplayLayerModes& displayLayerModes,
    ProxyRenderDelegate&)
{
    displayLayerModes = DisplayLayerModes();
}
#endif

void MayaUsdRPrim::_SyncDisplayLayerModes(SdfPath const& id, bool instancedPrim)
{
    auto* const          param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    ProxyRenderDelegate& drawScene = param->GetDrawScene();

    // First check if the status need updating
    if (drawScene.GetFrameCounter() == _displayLayerModesFrame) {
        return;
    }

    _displayLayerModesFrame = drawScene.GetFrameCounter();

    // Obtain scene prim path
    SdfPath usdPath;

    if (instancedPrim) {
        auto delegate = drawScene.GetUsdImagingDelegate();
        auto instancerId = delegate->GetInstancerId(id);

        VtIntArray indices = delegate->GetInstanceIndices(instancerId, id);
        const int  instanceIndex = indices.empty() ? 0 : indices[0];

        // The additional condition below is to prevent a crash in USD function GetScenePrimPath
        bool instanced = !instancerId.IsEmpty();
        instanced = (instanced && !indices.empty());

        HdInstancerContext instancerContext;
        usdPath = instanced ? drawScene.GetScenePrimPath(id, instanceIndex, &instancerContext)
                            : SdfPath();

        // Native instances use per-instance _displayLayerModes
        if (instancerContext.empty()) {
            _useInstancedDisplayLayerModes = true;
            _displayLayerModes = DisplayLayerModes();
            return;
        }
    } else {
        const int instanceIndex = UsdImagingDelegate::ALL_INSTANCES;
        usdPath = drawScene.GetScenePrimPath(id, instanceIndex, nullptr);
    }

    // Otherwise, populate display layer modes
    _useInstancedDisplayLayerModes = false;
    _PopulateDisplayLayerModes(usdPath, _displayLayerModes, drawScene);
}

void MayaUsdRPrim::_SyncDisplayLayerModesInstanced(SdfPath const& id, unsigned int instanceCount)
{
    auto* const          param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    ProxyRenderDelegate& drawScene = param->GetDrawScene();

    // First check if the status need updating
    if (drawScene.GetFrameCounter() == _displayLayerModesInstancedFrame) {
        return;
    }

    _displayLayerModesInstancedFrame = drawScene.GetFrameCounter();

    _forcedReprFlags = 0;
    _requiredModFlagsBitset.reset();
    if (_useInstancedDisplayLayerModes) {
        _displayLayerModesInstanced.resize(instanceCount);

        const SdfPathVector usdPaths = drawScene.GetScenePrimPaths(id, instanceCount);

        for (unsigned int usdInstanceId = 0; usdInstanceId < instanceCount; usdInstanceId++) {
            auto&          displayLayerModes = _displayLayerModesInstanced[usdInstanceId];
            const SdfPath& usdPath = usdPaths[usdInstanceId];
            _PopulateDisplayLayerModes(usdPath, displayLayerModes, drawScene);

            if (displayLayerModes._reprOverride == kBBox) {
                _forcedReprFlags |= kForcedBBox;
            } else if (displayLayerModes._reprOverride == kWire) {
                _forcedReprFlags |= kForcedWire;
            } else if (!displayLayerModes._texturing) {
                _forcedReprFlags |= kForcedUntextured;
            }

            int requiredModFlags = 0;
            if (displayLayerModes._hideOnPlayback) {
                requiredModFlags |= HdVP2DrawItem::kHideOnPlayback;
            }
            if (displayLayerModes._displayType != kNormal) {
                requiredModFlags |= HdVP2DrawItem::kUnselectable;
            }
            _requiredModFlagsBitset.set(requiredModFlags);
        }
    } else {
        _displayLayerModesInstanced.clear();
    }
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
        sharedData.visible = delegate->GetVisible(id) && _displayLayerModes._visibility;

        // Invisible rprims don't get calls to Sync or _PropagateDirtyBits while
        // they are invisible. This means that when a prim goes from visible to
        // invisible that we must update every repr, because if we switch reprs while
        // invisible we'll get no chance to update!
        if (!sharedData.visible)
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
    }

    // If instancer is dirty, update instancing map
    if (HdChangeTracker::IsInstancerDirty(*dirtyBits, id)) {
        bool instanced = !refThis.GetInstancerId().IsEmpty();
        // The additional condition below is to prevent a crash in USD function GetScenePrimPath
        instanced
            = (instanced && !delegate->GetInstanceIndices(refThis.GetInstancerId(), id).empty());

        // UpdateInstancingMapEntry is not multithread-safe, so enqueue the call
        _delegate->GetVP2ResourceRegistry().EnqueueCommit([this, id, instanced]() {
            auto* const param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
            auto&       drawScene = param->GetDrawScene();

            InstancePrototypePath newPathInPrototype
                = instanced ? drawScene.GetPathInPrototype(id) : sVoidInstancePrototypePath;
            drawScene.UpdateInstancingMapEntry(_pathInPrototype, newPathInPrototype, id);
            _pathInPrototype = newPathInPrototype;
        });
    }

#if PXR_VERSION > 2111
    // Hydra now manages and caches render tags under the hood and is clearing
    // the dirty bit prior to calling sync. Unconditionally set the render tag
    // in the shared data structure based on current Hydra data
    _RenderTag() = renderTag;
#else
    if (*dirtyBits & (HdChangeTracker::DirtyRenderTag)) {
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
        *dirtyBits &= ~(HdChangeTracker::DirtyRenderTag);
        return false;
    }

    return true;
}

MColor
MayaUsdRPrim::_GetHighlightColor(const TfToken& className, HdVP2SelectionStatus selectionStatus)
{
    auto* const          param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    ProxyRenderDelegate& drawScene = param->GetDrawScene();

    if (_displayLayerModes._displayType == MayaUsdRPrim::kTemplate) {
        return drawScene.GetTemplateColor(selectionStatus != kUnselected);
    } else if (
        _displayLayerModes._displayType == MayaUsdRPrim::kReference
        && selectionStatus == kUnselected) {
        return drawScene.GetReferenceColor();
    } else {
        return (
            selectionStatus != kUnselected ? drawScene.GetSelectionHighlightColor(
                selectionStatus == kFullyLead ? TfToken() : className)
                                           : _GetWireframeColor());
    }
}

MColor MayaUsdRPrim::_GetHighlightColor(const TfToken& className)
{
    return _GetHighlightColor(className, _selectionStatus);
}

MColor MayaUsdRPrim::_GetWireframeColor()
{
    if (_displayLayerModes._wireframeColorIndex > 0) {
        return MColor(M3dView::active3dView().colorAtIndex(
            _displayLayerModes._wireframeColorIndex - 1, M3dView::kDormantColors));
    } else if (_displayLayerModes._wireframeColorIndex < 0) {
        return _displayLayerModes._wireframeColorRGBA;
    } else {
        auto* const param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
        return param->GetDrawScene().GetWireframeColor();
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

    // Register to be notified if the surface shader changes due to a topology change:
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

    return materialId;
}

bool MayaUsdRPrim::_GetMaterialPrimvars(
    HdRenderIndex& renderIndex,
    const SdfPath& materialId,
    TfTokenVector& primvars)
{
    const HdVP2Material* material = static_cast<const HdVP2Material*>(
        renderIndex.GetSprim(HdPrimTypeTokens->material, materialId));
    if (!material || !material->GetSurfaceShader(TfToken())) {
        return false;
    }

    // Get basic primvars
    primvars = material->GetRequiredPrimvars(TfToken());

    // Get extra primvars
    if (material->GetSurfaceShader(HdReprTokens->smoothHull)) {
        const auto& extraPrimvars = material->GetRequiredPrimvars(HdReprTokens->smoothHull);
        for (const auto& extraPrimvar : extraPrimvars) {
            if (std::find(primvars.begin(), primvars.end(), extraPrimvar) == primvars.end()) {
                primvars.push_back(extraPrimvar);
            }
        }
    }

    return true;
}

bool MayaUsdRPrim::_FilterInstanceByDisplayLayer(
    unsigned int           usdInstanceId,
    BasicWireframeColors&  instanceColor,
    const TfToken&         reprToken,
    int                    modFlags,
    bool                   isHighlightItem,
    bool                   isDedicatedHighlightItem,
    InstanceColorOverride& colorOverride) const
{
    if (_displayLayerModesInstanced.size() <= usdInstanceId) {
        return false;
    }

    // Verify display layer visibility
    const auto& displayLayerModes = _displayLayerModesInstanced[usdInstanceId];
    if (!displayLayerModes._visibility) {
        return true;
    }

    // Process draw mode overrides
    const bool forcedBboxItem = (reprToken == HdVP2ReprTokens->forcedBbox);
    const bool forcedWireItem = (reprToken == HdVP2ReprTokens->forcedWire);
    const bool forcedUntexturedItem = (reprToken == HdVP2ReprTokens->forcedUntextured);
    if (displayLayerModes._reprOverride == kNone) {
        if (displayLayerModes._texturing) {
            // In no-override mode, an instance should be drawn only by
            // the non-forced reprs, so skip the forced ones.
            if (forcedBboxItem || forcedWireItem || forcedUntexturedItem) {
                return true;
            }
        } else {
            // Untextured override cannot affect bbox and wire modes, so keep
            // those reprs along with the forcedUntextured one.
            // Also, since forcedUntextured repr doesn't have a dedicated highlight
            // draw item, it is drawn by non-forced reprs.
            bool bboxItem = reprToken == HdVP2ReprTokens->bbox;
            bool wireItem = reprToken == HdReprTokens->wire;
            if (!isDedicatedHighlightItem && !forcedUntexturedItem && !bboxItem && !wireItem) {
                return true;
            }
        }
    } else if (displayLayerModes._reprOverride == kWire) {
        // Wire override cannot affect bbox mode so keep this repr
        // along with the forcedWire one.
        bool bboxItem = reprToken == HdVP2ReprTokens->bbox;
        if (!forcedWireItem && !bboxItem) {
            return true;
        }
    } else if (displayLayerModes._reprOverride == kBBox) {
        // Bbox override affects all draw modes.
        if (!forcedBboxItem) {
            return true;
        }
    }

    // Match item's hide-on-playback mode against that of the instance
    const bool hideOnPlaybackItem = (modFlags & HdVP2DrawItem::kHideOnPlayback);
    if (displayLayerModes._hideOnPlayback != hideOnPlaybackItem) {
        return true;
    }

    // Match item's 'unselectable' mode against that of the instance
    const bool unselectableItem = (modFlags & HdVP2DrawItem::kUnselectable);
    const bool unselectableInstance = displayLayerModes._displayType != kNormal;
    if (unselectableInstance != unselectableItem) {
        return true;
    }

    // Template and reference modes may affect visibility and wireframe color of items
    if (displayLayerModes._displayType == kTemplate) {
        if (!isHighlightItem) {
            return true; // Solid geometry is not drawn in the template mode
        } else {
            instanceColor = (instanceColor == kDormant) ? kTemplateDormat : kTemplateActive;
        }
    } else if (displayLayerModes._displayType == kReference) {
        if (instanceColor == kDormant) {
            if (isDedicatedHighlightItem) {
                // Hide dedicated highlight items when unselected. Since 'template' and
                // 'reference' modes share the same mod, we have to keep dedicated highlight
                //  item generally enabled, and thus we have a special case here
                return true;
            } else {
                instanceColor = kReferenceDormat;
            }
        }
    }

    // Now that we know that this instance will be rendered, let's check for color override.
    if (colorOverride._allowed && instanceColor == kDormant) {
        if (displayLayerModes._wireframeColorIndex > 0) {
            colorOverride._enabled = true;
            colorOverride._color = MColor(M3dView::active3dView().colorAtIndex(
                displayLayerModes._wireframeColorIndex - 1, M3dView::kDormantColors));
        } else if (displayLayerModes._wireframeColorIndex < 0) {
            colorOverride._enabled = true;
            colorOverride._color = displayLayerModes._wireframeColorRGBA;
        }
    }

    return false;
}

void MayaUsdRPrim::_SyncForcedReprs(
    HdRprim&          refThis,
    HdSceneDelegate*  delegate,
    HdRenderParam*    renderParam,
    HdDirtyBits*      dirtyBits,
    ReprVector const& reprs)
{
    // Forced representations work only for instanced primitives
    if (refThis.GetInstancerId().IsEmpty()) {
        return;
    }

    auto* const          param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    ProxyRenderDelegate& drawScene = param->GetDrawScene();

    // First check if the sync still needs to be performed
    if (drawScene.GetFrameCounter() == _forcedReprsFrame) {
        return;
    }

    _forcedReprsFrame = drawScene.GetFrameCounter();

    RenderItemFunc hideDrawItem = [this](HdVP2DrawItem::RenderItemData& renderItemData) {
        if (renderItemData._enabled) {
            renderItemData._enabled = false;
            _delegate->GetVP2ResourceRegistry().EnqueueCommit(
                [&renderItemData]() { renderItemData._renderItem->enable(false); });
        }
    };

    if (_forcedReprFlags & kForcedBBox) {
        refThis.Sync(delegate, renderParam, dirtyBits, HdVP2ReprTokens->forcedBbox);
    } else {
        _ForEachRenderItemInRepr(_FindRepr(reprs, HdVP2ReprTokens->forcedBbox), hideDrawItem);
    }

    if (_forcedReprFlags & kForcedWire) {
        refThis.Sync(delegate, renderParam, dirtyBits, HdVP2ReprTokens->forcedWire);
    } else {
        _ForEachRenderItemInRepr(_FindRepr(reprs, HdVP2ReprTokens->forcedWire), hideDrawItem);
    }

    if (_forcedReprFlags & kForcedUntextured) {
        refThis.Sync(delegate, renderParam, dirtyBits, HdVP2ReprTokens->forcedUntextured);
    } else {
        _ForEachRenderItemInRepr(_FindRepr(reprs, HdVP2ReprTokens->forcedUntextured), hideDrawItem);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
