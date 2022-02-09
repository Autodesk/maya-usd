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
#ifndef HD_VP2_MAYA_PRIM_COMMON
#define HD_VP2_MAYA_PRIM_COMMON

#include "draw_item.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/types.h"

#include <mayaUsd/render/vp2RenderDelegate/proxyRenderDelegate.h>

#include <maya/MHWGeometry.h>

#include <tbb/concurrent_unordered_map.h>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HdVP2RenderDelegate;

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
// Each instanced render item needs to map from a Maya instance id
// back to a usd instance id.
using InstanceIdMap = std::vector<unsigned int>;

using InstancePrimPaths = std::vector<SdfPath>;

// global singleton rather than MUserData, because consolidated world will
// not consolidate render items with different MUserData objects.
class MayaUsdCustomData
{
public:
    struct MayaUsdRenderItemData
    {
        InstanceIdMap _instanceIdMap;
        bool          _itemDataDirty { false };
    };

    struct MayaUsdPrimData
    {
        InstancePrimPaths _instancePrimPaths;
    };

    tbb::concurrent_unordered_map<int, MayaUsdRenderItemData>              _itemData;
    tbb::concurrent_unordered_map<SdfPath, MayaUsdPrimData, SdfPath::Hash> _primData;

    static InstanceIdMap& Get(const MHWRender::MRenderItem& item);
    static void           Remove(const MHWRender::MRenderItem& item);
    static bool           ItemDataDirty(const MHWRender::MRenderItem& item);
    static void           ItemDataDirty(const MHWRender::MRenderItem& item, bool dirty);

    static InstancePrimPaths& GetInstancePrimPaths(const SdfPath& prim);
    static void               RemoveInstancePrimPaths(const SdfPath& prim);
};
#endif

//! A primvar vertex buffer data map indexed by primvar name.
using PrimvarBufferDataMap = std::unordered_map<TfToken, void*, TfToken::HashFunctor>;

//! \brief  Helper struct used to package all the changes into single commit task
//!         (such commit task will be executed on main-thread)
struct MayaUsdCommitState
{
    HdVP2DrawItem::RenderItemData& _renderItemData;

    //! If valid, new index buffer data to commit
    int* _indexBufferData { nullptr };
    //! If valid, new primvar buffer data to commit
    PrimvarBufferDataMap _primvarBufferDataMap;

    //! If valid, world matrix to set on the render item
    MMatrix* _worldMatrix { nullptr };

    //! If valid, bounding box to set on the render item
    MBoundingBox* _boundingBox { nullptr };

    //! if valid, enable or disable the render item
    bool* _enabled { nullptr };

    //! if valid, set the primitive type on the render item
    MHWRender::MGeometry::Primitive* _primitiveType { nullptr };
    //! if valid, set the primitive stride on the render item
    int* _primitiveStride { nullptr };

    //! Instancing doesn't have dirty bits, every time we do update, we must update instance
    //! transforms
    MMatrixArray _instanceTransforms;

    //! Color parameter that _instanceColors should be bound to
    MString _instanceColorParam;

    //! Color array to support per-instance color and selection highlight.
    MFloatArray _instanceColors;

    //! List of runtime paths that a render item represents
    MStringArray _ufeIdentifiers;

    //! If valid, new shader instance to set
    MHWRender::MShaderInstance* _shader { nullptr };

    //! Is this object transparent
    bool _isTransparent { false };

    //! If true, associate geometric buffers to the render item and trigger consolidation/instancing
    //! update
    bool _geometryDirty { false };

    //! Construct valid commit state
    MayaUsdCommitState(HdVP2DrawItem::RenderItemData& renderItemData)
        : _renderItemData(renderItemData)
    {
    }

    //! No default constructor, we need draw item and dirty bits.
    MayaUsdCommitState() = delete;

    //! returns true if there is no state to commit
    bool Empty()
    {
        return _indexBufferData == nullptr && _shader == nullptr && _enabled == nullptr
            && !_geometryDirty && _boundingBox == nullptr && !_renderItemData._usingInstancedDraw
            && _instanceTransforms.length() == 0 && _ufeIdentifiers.length() == 0
            && _worldMatrix == nullptr;
    }
};

class MayaUsdRPrim
{
public:
    enum DirtyBits : HdDirtyBits
    {
        // The rprim has been added, removed or otherwise changed such that the selection highlight
        // for the item is dirty
        DirtySelectionHighlight = HdChangeTracker::CustomBitsBegin,
        // Maya's selection mode has changed, for example into point snapping mode
        DirtySelectionMode = (DirtySelectionHighlight << 1),
        // Maya's display mode has changed, for example for shaded to wireframe
        DirtyDisplayMode = (DirtySelectionMode << 1),
        DirtyBitLast = DirtyDisplayMode
    };

    static const MColor       kOpaqueBlue;           //!< Opaque blue
    static const MColor       kOpaqueGray;           //!< The default 18% gray color
    static const unsigned int kNumColorChannels = 4; //!< The number of color channels

    //! Cached strings for efficiency
    static const MString kPositionsStr;
    static const MString kNormalsStr;
    static const MString kDiffuseColorStr;
    static const MString kSolidColorStr;

    MayaUsdRPrim(HdVP2RenderDelegate* delegate, const SdfPath& id);
    virtual ~MayaUsdRPrim() = default;

protected:
    using ReprVector = std::vector<std::pair<TfToken, HdReprSharedPtr>>;

    void _CommitMVertexBuffer(MHWRender::MVertexBuffer* const, void*) const;

    void _UpdateTransform(
        MayaUsdCommitState&      stateToCommit,
        const HdRprimSharedData& sharedData,
        const HdDirtyBits        itemDirtyBits,
        const bool               isBoundingBoxItem);

    void _FirstInitRepr(HdDirtyBits* dirtyBits, SdfPath const& id);

    void _SetDirtyRepr(const HdReprSharedPtr& repr);

    bool _SyncCommon(HdDirtyBits* dirtyBits, const SdfPath& id, HdReprSharedPtr const& curRepr, HdRenderIndex& renderIndex);

    void _SyncSharedData(HdRprimSharedData& sharedData, HdSceneDelegate* delegate, HdDirtyBits const* dirtyBits, TfToken const& reprToken, SdfPath const& id, ReprVector const& reprs);

    void _PropagateDirtyBitsCommon(HdDirtyBits& bits, const ReprVector& reprs) const;

    void _MakeOtherReprRenderItemsInvisible(const TfToken&, const ReprVector&);

    void _HideAllDrawItems(HdReprSharedPtr const& curRepr);

    //! Helper utility function to adapt Maya API changes.
    static void _SetWantConsolidation(MHWRender::MRenderItem& renderItem, bool state);

    MHWRender::MRenderItem* _CreateWireframeRenderItem(const MString& name, const MColor& color, const MSelectionMask& selectionMask, MUint64 exclusionFlag) const;
    MHWRender::MRenderItem* _CreateBoundingBoxRenderItem(const MString& name, const MColor& color, const MSelectionMask& selectionMask, MUint64 exclusionFlag) const;

#ifndef MAYA_NEW_POINT_SNAPPING_SUPPORT
    MHWRender::MRenderItem* _CreatePointsRenderItem(const MString& name, const MSelectionMask& selectionMask, MUint64 exclusionFlag) const;
#endif

    virtual TfToken& _RenderTag() = 0;

    //! VP2 render delegate for which this prim was created
    HdVP2RenderDelegate* _delegate { nullptr };

    //! Rprim id cached as a maya string for easier debugging and profiling
    const MString _rprimId;

    //! Selection status of the Rprim
    HdVP2SelectionStatus _selectionStatus { kUnselected };

    //! The string representation of the runtime only path to this object
    MStringArray _PrimSegmentString;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_VP2_MAYA_PRIM_COMMON
