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
#ifndef HD_VP2_SCENE_INDEX_PRODUCER
#define HD_VP2_SCENE_INDEX_PRODUCER

#include "sceneIndex/subtreeScopingSceneIndex.h"
#include "usdProducer.h"

#include <pxr/imaging/hd/noticeBatchingSceneIndex.h>
#include <pxr/imaging/hdsi/legacyDisplayStyleOverrideSceneIndex.h>
#include <pxr/imaging/hdsi/prefixPathPruningSceneIndex.h>
#include <pxr/imaging/hdx/selectionSceneIndexObserver.h>
#include <pxr/pxr.h>
#include <pxr/usdImaging/usdImaging/rootOverridesSceneIndex.h>
#include <pxr/usdImaging/usdImaging/selectionSceneIndex.h>
#include <pxr/usdImaging/usdImaging/stageSceneIndex.h>

#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdVP2DirtyingSceneIndex);

/*! \brief  A pass-through filter that lets VP2 invalidate its own prims.

    VP2 dirties rprims imperatively - for selection highlight, display mode,
    display layers, render tags and material changes. Under scene index
    emulation HdChangeTracker::MarkRprimDirty does not set those bits directly:
    it translates them to data source locators and re-injects them through the
    render index's HdLegacyPrimSceneIndex. That path ends in
    HdRetainedSceneIndex::DirtyPrims, which discards any entry whose path it
    does not hold. Prims contributed by HdRenderIndex::InsertSceneIndex are not
    in that table, so every such notice is dropped and the mark is a silent
    no-op. A legacy scene delegate like UsdImagingDelegate is unaffected,
    because its prims are in the table.

    Sitting at the terminal of the producer's chain, this scene index can emit
    the notice from a point that does own the prims, so it reaches the back-end
    emulation delegate and is translated back into dirty bits as usual.

    Marks raised during HdRenderIndex::SyncAll are held back by the inherited
    notice batching until the sync is over; see
    HdVP2SceneIndexProducer::_MarkDirty. While batching is on, every chain
    notice is held, not only VP2's marks. Nothing upstream emits in that window.
    Every SyncAll runs inside ProxyRenderDelegate::_Execute, which ends by
    flushing, and the upstream edits all come before its first sync: stage,
    time, root overrides and display style in _UpdateSceneDelegate, selection in
    _UpdateSelectionStates.

    \class  HdVP2DirtyingSceneIndex
*/
class HdVP2DirtyingSceneIndex final : public HdNoticeBatchingSceneIndex
{
public:
    static HdVP2DirtyingSceneIndexRefPtr New(const HdSceneIndexBaseRefPtr& inputSceneIndex)
    {
        return TfCreateRefPtr(new HdVP2DirtyingSceneIndex(inputSceneIndex));
    }

    //! \brief  Emits a PrimsDirtied notice for prims this chain contributes,
    //!         or holds it while batching is enabled.
    void DirtyPrims(const HdSceneIndexObserver::DirtiedPrimEntries& entries)
    {
        _PrimsDirtied(*this, entries);
    }

protected:
    using HdNoticeBatchingSceneIndex::HdNoticeBatchingSceneIndex;
};

/*! \brief  VP2 USD producer backed by a Hydra 2.0 scene index chain.

    Built only when CMAKE_WANT_MAYAUSD_VP2_USE_SCENE_INDEX=ON. The chain is fed
    into the render index with HdRenderIndex::InsertSceneIndex, so Hydra's scene
    index emulation wraps it in a scene delegate and the VP2 rprims keep seeing
    an HdSceneDelegate*. The render delegate, render index, task controller and
    Hydra engine are unchanged.

    Selecting a single point instance highlights that instance only, including
    what its prototypes instance again: native instances and nested point
    instancers. A point instancer the chain does not draw as an instancer, which
    picking never yields as a point instance, highlights all of its instances.
    See PopulateSelection.

    \class  HdVP2SceneIndexProducer
*/
class HdVP2SceneIndexProducer final : public HdVP2UsdProducer
{
public:
    HdVP2SceneIndexProducer() = default;
    ~HdVP2SceneIndexProducer() override;

    void Initialize(HdRenderIndex* renderIndex, const SdfPath& producerId) override;
    bool Populate(
        const UsdStageRefPtr& stage,
        const SdfPath&        rootPath,
        const SdfPathVector&  excludedPaths) override;

    void ApplyPendingUpdates() override;
    bool FlushDeferredUpdates() override;

    void        SetTime(const UsdTimeCode& timeCode) override;
    UsdTimeCode GetTime() const override;

    void       SetRootTransform(const GfMatrix4d& transform) override;
    GfMatrix4d GetRootTransform() const override;

    void SetRootVisibility(bool visible) override;
    bool GetRootVisibility() const override;

    void SetRefineLevelFallback(int level) override;
    int  GetRefineLevelFallback() const override;

    SdfPath ToIndexPath(const SdfPath& usdPath) const override;

    SdfPath GetScenePrimPath(
        const SdfPath&      rprimId,
        int                 instanceIndex,
        HdInstancerContext* instancerContext) const override;

    SdfPathVector GetScenePrimPaths(
        const SdfPath&          rprimId,
        const std::vector<int>& instanceIndexes) const override;

    void PopulateSelection(
        const SdfPath&              usdPath,
        int                         instanceIndex,
        const HdSelectionSharedPtr& result) override;

    void ClearSelection() override;

    void MarkRprimDirty(const SdfPath& indexPath, HdDirtyBits bits) override;
    void MarkSprimDirty(const SdfPath& indexPath, HdDirtyBits bits) override;

    HdSceneDelegate*    GetSceneDelegate() const override;
    UsdImagingDelegate* GetUsdImagingDelegate() const override;

private:
    /*! \brief  An rprim a point instancer draws, as point instance selection
                needs it.

        Two index spaces meet in point instance selection. An *instance index*
        indexes UsdGeomPointInstancer's protoIndices and positions; that is what
        UFE carries in a point instance path and what reaches
        PopulateSelection. An *instance id* is a position in the array of
        instances an rprim is drawn with; that is what HdSelection::AddInstance
        takes, and what HdVP2Mesh and HdVP2Instancer index with.

        The point instance with instance index i has id k, the position of i in
        HdInstancerTopologySchema::ComputeInstanceIndicesForProto(prototypeRoot).
        An rprim instanced again below the point instancer - by a native
        instance or a nested point instancer in the prototype - is drawn stride
        times per point instance, with ids [k * stride, (k + 1) * stride).
    */
    struct _PointInstanceRprim
    {
        SdfPath indexPath;     //!< The rprim.
        SdfPath prototypeRoot; //!< The point instancer's prototype root above it.
        int     stride;        //!< Instances drawn per point instance.
    };

    using _PointInstanceRprims = std::vector<_PointInstanceRprim>;

    /*! \brief  The rprims a point instancer draws, cached until ClearSelection.

        \return nullptr when the chain does not draw instancer as an instancer,
                which tells PopulateSelection to resolve the item with
                AddSelection instead.
    */
    const _PointInstanceRprims* _PointInstanceRprimsOf(const SdfPath& instancer);

    /*! \brief  Instance index to instance id, for the instances of
                prototypeRoot, cached until ClearSelection.

        A map rather than a search over the array, because one selection pass
        can ask about tens of thousands of instances of one instancer.
    */
    const std::unordered_map<int, int>&
    _InstanceIds(const SdfPath& instancer, const SdfPath& prototypeRoot);

    /*! \brief  Converts a render index path back into a path in this chain.

        The exact inverse of ToIndexPath, and a different question from the one
        GetScenePrimPath answers - see the comment on the definition.

        \return An empty path for a path outside producerId.
    */
    SdfPath _ToChainPath(const SdfPath& indexPath) const;

    //! HdDirtyBitsTranslator::RprimDirtyBitsToLocatorSet or its Sprim twin.
    using _ToLocators = void (*)(const TfToken&, HdDirtyBits, HdDataSourceLocatorSet*);

    /*! \brief  Emits the invalidation for a prim this producer contributed, or
                holds it back while Hydra is mid-sync.

        HdSceneIndexAdapterSceneDelegate::PrimsDirtied must not run during
        HdRenderIndex::SyncAll: it opens with TF_VERIFY(!IsSyncAllInProgress())
        and clears the per-thread input prim cache the parallel rprim sync
        inserts into. VP2 marks from inside Sync in one place: HdVP2Material's
        CompiledNetwork::Sync tells the rprims bound to the material to pick up
        a new shader. Such marks are held back; ProxyRenderDelegate::_Execute
        calls FlushDeferredUpdates once the sync returns and runs one more for
        them, so they still land in the same frame.

        The batching that holds them does no locking, which relies on marks
        coming from the main thread only: materials sync serially because
        HdVP2RenderDelegate keeps the default
        HdRenderDelegate::IsParallelSyncEnabled.
    */
    void _MarkDirty(const SdfPath& indexPath, HdDirtyBits bits, _ToLocators toLocators);

    HdRenderIndex* _renderIndex { nullptr };
    SdfPath        _producerId;

    UsdImagingStageSceneIndexRefPtr                _stageSceneIndex;
    HdNoticeBatchingSceneIndexRefPtr               _noticeBatchingSceneIndex;
    UsdImagingSelectionSceneIndexRefPtr            _selectionSceneIndex;
    UsdImagingRootOverridesSceneIndexRefPtr        _rootOverrides;
    HdsiLegacyDisplayStyleOverrideSceneIndexRefPtr _displayStyle;

    //! Scopes the chain to the proxy shape's primPath. Sits on stage paths,
    //! ahead of instancing. See Initialize.
    HdVP2SubtreeScopingSceneIndexRefPtr _subtreeScoping;

    //! Prunes the proxy shape's excludePrimPaths, subtree and all. Sits on stage
    //! paths between the subtree scoping and the root overrides, so instancing,
    //! material binding resolution and selection never see an excluded prim.
    //! Unlike with UsdImagingDelegate, excluded prims also stop serving reads by
    //! path: a collection binding whose collection an excluded prim owns, or a
    //! skel:animationSource naming one, no longer resolves.
    //! One exception, an OpenUSD bug still present in 26.08: a notice batch in
    //! which every entry is excluded is forwarded unfiltered, so an update whose
    //! resyncs all fall in excluded subtrees adds those prims anyway. They have
    //! no data and draw nothing. See Initialize.
    HdsiPrefixPathPruningSceneIndexRefPtr _exclusionPruning;

    //! Chain terminal, and the scene index handed to InsertSceneIndex.
    HdVP2DirtyingSceneIndexRefPtr _dirtying;

    //! Turns the HdSelectionsSchema that _selectionSceneIndex stamps onto prims
    //! back into an HdSelection.
    HdxSelectionSceneIndexObserver _selectionObserver;

    //! Caches for point instance selection, valid for one selection pass. An
    //! empty optional records an instancer that cannot be resolved, so it is
    //! probed only once per pass.
    std::map<SdfPath, std::optional<_PointInstanceRprims>> _pointInstanceRprims;
    std::map<SdfPath, std::unordered_map<int, int>>        _instanceIds;

    //! HdsiLegacyDisplayStyleOverrideSceneIndex is set-only, so the current
    //! fallback is tracked here. 0 matches UsdImagingDelegate's default.
    int _refineLevelFallback { 0 };
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
