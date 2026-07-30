//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// This is a modified version of the hdPrman render pass scene index, reduced to
// the "prune" and "renderVisibility" collections. The "cameraVisibility" and
// "matte" collections were dropped because VP2 has no renderer-neutral concept
// to map them onto.
//
#ifndef MAYAUSD_VP2_RENDER_PASS_SCENE_INDEX_H
#define MAYAUSD_VP2_RENDER_PASS_SCENE_INDEX_H

#include <pxr/imaging/hd/collectionExpressionEvaluator.h>
#include <pxr/imaging/hd/filteringSceneIndex.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/pathExpression.h>

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(MayaUsdRenderPassSceneIndex);

/// Applies the active render pass named in the HdSceneGlobalsSchema: prims
/// matching the pass's "prune" collection are removed from the scene, and
/// renderable prims outside its "renderVisibility" collection are made
/// invisible.
///
/// \note Like the hdPrman original, this assumes the active render pass is a
///       UsdRenderPass for the purposes of collection naming conventions.
///
class MayaUsdRenderPassSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    static MayaUsdRenderPassSceneIndexRefPtr New(const HdSceneIndexBaseRefPtr& inputSceneIndex);

    HdSceneIndexPrim GetPrim(const SdfPath& primPath) const override;
    SdfPathVector    GetChildPrimPaths(const SdfPath& primPath) const override;

protected:
    MayaUsdRenderPassSceneIndex(const HdSceneIndexBaseRefPtr& inputSceneIndex);
    ~MayaUsdRenderPassSceneIndex() override;

    void _PrimsAdded(
        const HdSceneIndexBase&                       sender,
        const HdSceneIndexObserver::AddedPrimEntries& entries) override;
    void _PrimsRemoved(
        const HdSceneIndexBase&                         sender,
        const HdSceneIndexObserver::RemovedPrimEntries& entries) override;
    void _PrimsDirtied(
        const HdSceneIndexBase&                         sender,
        const HdSceneIndexObserver::DirtiedPrimEntries& entries) override;

private:
    /// State derived from the active render pass. An empty renderPassPath means
    /// no pass is active. Evaluators are set sparsely, matching the presence of
    /// each collection on the pass prim.
    struct _RenderPassState
    {
        SdfPath renderPassPath;

        // Retained so old and new state can be compared.
        SdfPathExpression renderVisExpr;
        SdfPathExpression pruneExpr;

        std::optional<HdCollectionExpressionEvaluator> renderVisEval;
        std::optional<HdCollectionExpressionEvaluator> pruneEval;

        bool DoesOverrideVis(const SdfPath& primPath, const HdSceneIndexPrim& prim) const;
        bool DoesPrune(const SdfPath& primPath) const;
    };

    void _UpdateActiveRenderPassState(
        HdSceneIndexObserver::AddedPrimEntries*   addedEntries,
        HdSceneIndexObserver::DirtiedPrimEntries* dirtyEntries,
        HdSceneIndexObserver::RemovedPrimEntries* removedEntries);

    _RenderPassState _activeRenderPass;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAUSD_VP2_RENDER_PASS_SCENE_INDEX_H
