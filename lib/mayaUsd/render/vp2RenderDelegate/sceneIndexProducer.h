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

#include "usdProducer.h"

#include <pxr/imaging/hd/noticeBatchingSceneIndex.h>
#include <pxr/imaging/hdsi/legacyDisplayStyleOverrideSceneIndex.h>
#include <pxr/imaging/hdx/selectionSceneIndexObserver.h>
#include <pxr/pxr.h>
#include <pxr/usdImaging/usdImaging/rootOverridesSceneIndex.h>
#include <pxr/usdImaging/usdImaging/selectionSceneIndex.h>
#include <pxr/usdImaging/usdImaging/stageSceneIndex.h>

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

    Batching is never enabled, so the inherited HdNoticeBatchingSceneIndex
    forwards every notice unchanged.

    \class  HdVP2DirtyingSceneIndex
*/
class HdVP2DirtyingSceneIndex final : public HdNoticeBatchingSceneIndex
{
public:
    static HdVP2DirtyingSceneIndexRefPtr New(const HdSceneIndexBaseRefPtr& inputSceneIndex)
    {
        return TfCreateRefPtr(new HdVP2DirtyingSceneIndex(inputSceneIndex));
    }

    //! \brief  Emits a PrimsDirtied notice for prims this chain contributes.
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

    void MarkRprimDirty(const SdfPath& indexPath, HdDirtyBits bits) override;
    void MarkSprimDirty(const SdfPath& indexPath, HdDirtyBits bits) override;

    HdSceneDelegate*    GetSceneDelegate() const override;
    UsdImagingDelegate* GetUsdImagingDelegate() const override;

private:
    /*! \brief  Converts a render index path back into a path in this chain.

        The exact inverse of ToIndexPath, and a different question from the one
        GetScenePrimPath answers - see the comment on the definition.

        \return An empty path for a path outside producerId.
    */
    SdfPath _ToChainPath(const SdfPath& indexPath) const;

    //! HdDirtyBitsTranslator::RprimDirtyBitsToLocatorSet or its Sprim twin.
    using _ToLocators = void (*)(const TfToken&, HdDirtyBits, HdDataSourceLocatorSet*);

    //! \brief  Emits the invalidation for a prim this producer contributed.
    void _MarkDirty(const SdfPath& indexPath, HdDirtyBits bits, _ToLocators toLocators);

    HdRenderIndex* _renderIndex { nullptr };
    SdfPath        _producerId;

    UsdImagingStageSceneIndexRefPtr                _stageSceneIndex;
    HdNoticeBatchingSceneIndexRefPtr               _noticeBatchingSceneIndex;
    UsdImagingSelectionSceneIndexRefPtr            _selectionSceneIndex;
    UsdImagingRootOverridesSceneIndexRefPtr        _rootOverrides;
    HdsiLegacyDisplayStyleOverrideSceneIndexRefPtr _displayStyle;

    //! Chain terminal, and the scene index handed to InsertSceneIndex.
    HdVP2DirtyingSceneIndexRefPtr _dirtying;

    //! Turns the HdSelectionsSchema that _selectionSceneIndex stamps onto prims
    //! back into an HdSelection.
    HdxSelectionSceneIndexObserver _selectionObserver;

    //! HdsiLegacyDisplayStyleOverrideSceneIndex is set-only, so the current
    //! fallback is tracked here. 0 matches UsdImagingDelegate's default.
    int _refineLevelFallback { 0 };
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
