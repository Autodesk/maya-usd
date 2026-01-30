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

#include "drawItem.h"

#include <mayaUsd/render/vp2RenderDelegate/proxyRenderDelegate.h>

#include <pxr/imaging/hd/changeTracker.h>
#include <pxr/imaging/hd/types.h>

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
using PrimvarBufferDataMap = std::unordered_map<TfToken, std::vector<char>, TfToken::HashFunctor>;

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
    const MMatrix* _worldMatrix { nullptr };

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
    std::shared_ptr<MMatrixArray> _instanceTransforms;

    //! Color parameter that _instanceColors should be bound to
    MString _instanceColorParam;

    //! Color array to support per-instance color and selection highlight.
    std::shared_ptr<MFloatArray> _instanceColors;

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
            && !_geometryDirty && _boundingBox == nullptr && !_instanceTransforms
            && !_instanceColors && _ufeIdentifiers.length() == 0 && _worldMatrix == nullptr;
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
        // 1 or more of Maya's display layers have changed, so the compound effect
        // of display layers on this prim will need to be recomputed
        DirtyDisplayLayers = (DirtyDisplayMode << 1),
        DirtyBitLast = DirtyDisplayLayers
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
    virtual ~MayaUsdRPrim();

protected:
    using ReprVector = std::vector<std::pair<TfToken, HdReprSharedPtr>>;
    using RenderItemFunc = std::function<void(HdVP2DrawItem::RenderItemData&)>;
    using UpdatePrimvarInfoFunc = std::function<
        void(const TfToken& name, const VtValue& value, const HdInterpolation interpolation)>;
    using ErasePrimvarInfoFunc = std::function<void(const TfToken& name)>;

    enum DisplayType
    {
        kNormal = 0,
        kTemplate = 1,
        kReference = 2
    };

    enum ReprOverride
    {
        kNone = 0,
        kBBox = 1,
        kWire = 2
    };

    enum ForcedReprFlags
    {
        kForcedBBox = 1 << 0,
        kForcedWire = 1 << 1,
        kForcedUntextured = 1 << 2
    };

    struct DisplayLayerModes
    {
        //! Requested display layer visibility
        bool _visibility { true };

        //! Requested HideOnPlayback status
        bool _hideOnPlayback { false };

        //! Defines representation override that should be applied to the prim
        ReprOverride _reprOverride { kNone };

        //! Requested display type of the Rprim
        DisplayType _displayType { kNormal };

        //! Requested texturing status
        bool _texturing { true };

        //! Wireframe color index
        // zero - override is disabled
        // negative - override with RGB color
        // positive - override with the given index
        int _wireframeColorIndex { 0 };

        //! Wireframe color override
        MColor _wireframeColorRGBA;
    };

    enum BasicWireframeColors : unsigned char
    {
        kDormant = 0,
        kActive = 1,
        kLead = 2,
        kTemplateDormat = 3,
        kTemplateActive = 4,
        kReferenceDormat = 5,

        kInvalid = 255
    };

    struct InstanceColorOverride
    {
        MColor     _color;
        bool       _enabled { false };
        const bool _allowed;

        InstanceColorOverride(bool allowed)
            : _allowed(allowed)
        {
        }

        void Reset() { _enabled = false; }
    };

    static void
    _ProcessDisplayLayerModes(const MObject& displayLayerObj, DisplayLayerModes& displayLayerModes);

    static void _PopulateDisplayLayerModes(
        const SdfPath&       usdPath,
        DisplayLayerModes&   displayLayerModes,
        ProxyRenderDelegate& drawScene);

    static HdReprSharedPtr _FindRepr(const ReprVector& reprs, const TfToken& reprToken);

    void _CommitMVertexBuffer(MHWRender::MVertexBuffer* const, void*) const;

    void _UpdateTransform(
        MayaUsdCommitState&      stateToCommit,
        const HdRprimSharedData& sharedData,
        const HdDirtyBits        itemDirtyBits,
        const bool               isBoundingBoxItem);

    void _FirstInitRepr(HdDirtyBits* dirtyBits, SdfPath const& id);

    void _SetDirtyRepr(const HdReprSharedPtr& repr);

    void _UpdateReprOverrides(ReprVector& reprs);

    TfToken _GetOverrideToken(TfToken const& reprToken) const;

    TfToken _GetMaterialNetworkToken(const TfToken& reprToken) const;

    HdReprSharedPtr _InitReprCommon(
        HdRprim&       refThis,
        TfToken const& reprToken,
        ReprVector&    reprs,
        HdDirtyBits*   dirtyBits,
        SdfPath const& id);

    bool _SyncCommon(
        HdRprim&               refThis,
        HdSceneDelegate*       delegate,
        HdRenderParam*         renderParam,
        HdDirtyBits*           dirtyBits,
        HdReprSharedPtr const& curRepr,
        TfToken const&         reprToken);

    void _SyncSharedData(
        HdRprimSharedData& sharedData,
        HdSceneDelegate*   delegate,
        HdDirtyBits const* dirtyBits,
        TfToken const&     reprToken,
        HdRprim const&     refThis,
        ReprVector const&  reprs,
        TfToken const&     renderTag);

    void _SyncDisplayLayerModes(SdfPath const& id, bool instancedPrim);
    void _SyncDisplayLayerModesInstanced(SdfPath const& id, unsigned int instanceCount);

    bool _FilterInstanceByDisplayLayer(
        unsigned int           usdInstanceId,
        BasicWireframeColors&  instanceColor,
        const TfToken&         reprToken,
        int                    modFlags,
        bool                   isHighlightItem,
        bool                   isDedicatedHighlightItem,
        InstanceColorOverride& colorOverride) const;

    void _SyncForcedReprs(
        HdRprim&          refThis,
        HdSceneDelegate*  delegate,
        HdRenderParam*    renderParam,
        HdDirtyBits*      dirtyBits,
        ReprVector const& reprs);

    void _UpdatePrimvarSourcesGeneric(
        HdSceneDelegate*       sceneDelegate,
        HdDirtyBits            dirtyBits,
        const TfTokenVector&   requiredPrimvars,
        HdRprim&               refThis,
        UpdatePrimvarInfoFunc& updatePrimvarInfo,
        ErasePrimvarInfoFunc&  erasePrimvarInfo);

    SdfPath _GetUpdatedMaterialId(HdRprim* rprim, HdSceneDelegate* delegate);
    MColor  _GetHighlightColor(const TfToken& className);
    MColor  _GetHighlightColor(const TfToken& className, HdVP2SelectionStatus selectionStatus);
    MColor  _GetWireframeColor();

    void _PropagateDirtyBitsCommon(HdDirtyBits& bits, const ReprVector& reprs) const;

    void _MakeOtherReprRenderItemsInvisible(const TfToken&, const ReprVector&);

    void _HideAllDrawItems(HdReprSharedPtr const& curRepr);

    static void _ForEachRenderItemInRepr(const HdReprSharedPtr& curRepr, RenderItemFunc& func);
    static void _ForEachRenderItem(const ReprVector& reprs, RenderItemFunc& func);

    //! Helper utility function to adapt Maya API changes.
    static void _SetWantConsolidation(MHWRender::MRenderItem& renderItem, bool state);

    static bool _GetMaterialPrimvars(HdRenderIndex&, const SdfPath&, TfTokenVector&);

    void _InitRenderItemCommon(MHWRender::MRenderItem* renderItem) const;

    HdVP2DrawItem::RenderItemData& _AddRenderItem(
        HdVP2DrawItem&          drawItem,
        MHWRender::MRenderItem* renderItem,
        MSubSceneContainer&     subSceneContainer,
        const HdGeomSubset*     geomSubset = nullptr) const;

    MHWRender::MRenderItem* _CreateWireframeRenderItem(
        const MString&        name,
        const MColor&         color,
        const MSelectionMask& selectionMask,
        MUint64               exclusionFlag) const;
    MHWRender::MRenderItem* _CreateBoundingBoxRenderItem(
        const MString&        name,
        const MColor&         color,
        const MSelectionMask& selectionMask,
        MUint64               exclusionFlag) const;

#ifndef MAYA_NEW_POINT_SNAPPING_SUPPORT
    MHWRender::MRenderItem* _CreatePointsRenderItem(
        const MString&        name,
        const MSelectionMask& selectionMask,
        MUint64               exclusionFlag) const;
#endif

    virtual TfToken& _RenderTag() = 0;

    //! VP2 render delegate for which this prim was created
    HdVP2RenderDelegate* _delegate { nullptr };

    //! Rprim id in Hydra
    const SdfPath _hydraId;

    //! Rprim id cached as a maya string for easier debugging and profiling
    const MString _rprimId;

    //! Selection status of the Rprim
    HdVP2SelectionStatus _selectionStatus { kUnselected };

    //! Modes requested by display layer along with the frame they are updated on
    bool                           _useInstancedDisplayLayerModes { false };
    DisplayLayerModes              _displayLayerModes;
    std::vector<DisplayLayerModes> _displayLayerModesInstanced;
    uint64_t                       _displayLayerModesFrame { 0 };
    uint64_t                       _displayLayerModesInstancedFrame { 0 };

    // For instanced primitives, specifies which mods are required
    std::bitset<HdVP2DrawItem::kModFlagsBitsetSize> _requiredModFlagsBitset;

    // forced representations runtime state
    int      _forcedReprFlags { 0 };
    uint64_t _forcedReprsFrame { 0 };

    //! HideOnPlayback status of the Rprim
    bool _hideOnPlayback { false };

    //! Representation override applied to the prim, if any
    ReprOverride _reprOverride { kNone };

    //! The string representation of the runtime only path to this object
    MStringArray _PrimSegmentString;

    //! For instanced prim, holds the corresponding path in USD prototype
    InstancePrototypePath _pathInPrototype { SdfPath(), kNativeInstancing };
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_VP2_MAYA_PRIM_COMMON
