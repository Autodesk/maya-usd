//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// This is a modified version of the hdPrman render pass scene index, reduced to
// the "prune" and "renderVisibility" collections.
//
#include "renderPassSceneIndex.h"

#include "debugCodes.h"

#include <pxr/base/trace/trace.h>
#include <pxr/imaging/hd/collectionSchema.h>
#include <pxr/imaging/hd/collectionsSchema.h>
#include <pxr/imaging/hd/dataSourceLocator.h>
#include <pxr/imaging/hd/overlayContainerDataSource.h>
#include <pxr/imaging/hd/retainedDataSource.h>
#include <pxr/imaging/hd/sceneGlobalsSchema.h>
#include <pxr/imaging/hd/sceneIndexPrimView.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hd/visibilitySchema.h>
#include <pxr/imaging/hdsi/utils.h>

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (renderVisibility)
    (prune)
);
// clang-format on

/* static */
MayaUsdRenderPassSceneIndexRefPtr
MayaUsdRenderPassSceneIndex::New(const HdSceneIndexBaseRefPtr& inputSceneIndex)
{
    return TfCreateRefPtr(new MayaUsdRenderPassSceneIndex(inputSceneIndex));
}

MayaUsdRenderPassSceneIndex::MayaUsdRenderPassSceneIndex(
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
    : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
    SetDisplayName("MayaUsd: render passes");
}

MayaUsdRenderPassSceneIndex::~MayaUsdRenderPassSceneIndex() = default;

static bool _IsGeometryType(const TfToken& primType)
{
    // Gprim types beyond those covered by HdPrimTypeIsGprim().
    static const TfTokenVector extraGeomTypes
        = { HdPrimTypeTokens->cone, HdPrimTypeTokens->cylinder, HdPrimTypeTokens->sphere };
    return HdPrimTypeIsGprim(primType)
        || std::find(extraGeomTypes.begin(), extraGeomTypes.end(), primType) != extraGeomTypes.end();
}

// Returns true if the renderVisibility rules apply to this prim type.
static bool _ShouldApplyPassVisibility(const TfToken& primType)
{
    return _IsGeometryType(primType) || HdPrimTypeIsLight(primType)
        || primType == HdPrimTypeTokens->lightFilter;
}

static bool _IsVisible(const HdContainerDataSourceHandle& primSource)
{
    if (const HdVisibilitySchema visSchema = HdVisibilitySchema::GetFromParent(primSource)) {
        if (const HdBoolDataSourceHandle visDs = visSchema.GetVisibility()) {
            return visDs->GetTypedValue(0.0f);
        }
    }
    return true;
}

bool MayaUsdRenderPassSceneIndex::_RenderPassState::DoesOverrideVis(
    const SdfPath&          primPath,
    const HdSceneIndexPrim& prim) const
{
    return renderVisEval && _ShouldApplyPassVisibility(prim.primType)
        && !renderVisEval->Match(primPath) && _IsVisible(prim.dataSource);
}

bool MayaUsdRenderPassSceneIndex::_RenderPassState::DoesPrune(const SdfPath& primPath) const
{
    return pruneEval && pruneEval->Match(primPath);
}

HdSceneIndexPrim MayaUsdRenderPassSceneIndex::GetPrim(const SdfPath& primPath) const
{
    // Pruning is also applied in GetChildPrimPaths(); doing it here as well
    // keeps a prim pruned even if a downstream scene index asks for it by path.
    if (_activeRenderPass.DoesPrune(primPath)) {
        return HdSceneIndexPrim();
    }

    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    // Renderable prims that are visible upstream but excluded from the pass's
    // renderVisibility collection get their visibility overridden to 0.
    if (_activeRenderPass.DoesOverrideVis(primPath, prim)) {
        static const HdContainerDataSourceHandle invisDs = HdRetainedContainerDataSource::New(
            HdVisibilitySchema::GetSchemaToken(),
            HdVisibilitySchema::Builder()
                .SetVisibility(HdRetainedTypedSampledDataSource<bool>::New(false))
                .Build());
        prim.dataSource = HdOverlayContainerDataSource::New(invisDs, prim.dataSource);
    }

    return prim;
}

SdfPathVector MayaUsdRenderPassSceneIndex::GetChildPrimPaths(const SdfPath& primPath) const
{
    if (_activeRenderPass.pruneEval) {
        SdfPathVector childPathVec = _GetInputSceneIndex()->GetChildPrimPaths(primPath);
        HdsiUtilsRemovePrunedChildren(primPath, *_activeRenderPass.pruneEval, &childPathVec);
        return childPathVec;
    }
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

/*

General notes on change processing and invalidation:

- Rather than lazily evaluate the active render pass state, and be prepared to
  do so from multiple caller threads, we instead greedily set up the active
  render pass state. Though greedy, this is a small amount of computation, and
  only triggered on changes to two specific scene locations: the root scope
  where HdSceneGlobalsSchema lives, and the scope where the designated active
  render pass lives.

- The list of entries for prims added, dirtied, or removed must be filtered
  against the active render pass prune collection.

- The list of entries for prims added, dirtied, or removed can imply changes to
  which render pass is active, or to the contents of the active render pass. In
  either case, if the effective render pass state changes, downstream observers
  must be notified about the effects.

*/

// Helper to scan an entry vector for an entry that could affect the active pass.
template <typename ENTRIES>
inline static bool _EntryCouldAffectPass(const ENTRIES& entries, const SdfPath& activeRenderPassPath)
{
    for (const auto& entry : entries) {
        // The prim at the root path contains the HdSceneGlobalsSchema.
        // The prim at the render pass path controls its behavior.
        if (entry.primPath.IsAbsoluteRootPath() || entry.primPath == activeRenderPassPath) {
            return true;
        }
    }
    return false;
}

// Helper to apply pruning to an entry list. Returns true if any pruning was
// applied, putting surviving entries into *postPruneEntries.
template <typename ENTRIES>
inline static bool _PruneEntries(
    std::optional<HdCollectionExpressionEvaluator>& pruneEval,
    const ENTRIES&                                  entries,
    ENTRIES*                                        postPruneEntries)
{
    if (!pruneEval) {
        return false;
    }
    bool foundEntryToPrune = false;
    for (const auto& entry : entries) {
        if (pruneEval->Match(entry.primPath)) {
            foundEntryToPrune = true;
            break;
        }
    }
    if (!foundEntryToPrune) {
        return false;
    }
    for (const auto& entry : entries) {
        if (!pruneEval->Match(entry.primPath)) {
            postPruneEntries->push_back(entry);
        }
    }
    return true;
}

void MayaUsdRenderPassSceneIndex::_PrimsAdded(
    const HdSceneIndexBase& /* sender */,
    const HdSceneIndexObserver::AddedPrimEntries& entries)
{
    HdSceneIndexObserver::AddedPrimEntries   extraAddedEntries;
    HdSceneIndexObserver::DirtiedPrimEntries extraDirtyEntries;
    HdSceneIndexObserver::RemovedPrimEntries extraRemovedEntries;

    if (_EntryCouldAffectPass(entries, _activeRenderPass.renderPassPath)) {
        _UpdateActiveRenderPassState(&extraAddedEntries, &extraDirtyEntries, &extraRemovedEntries);
    }

    if (!_PruneEntries(_activeRenderPass.pruneEval, entries, &extraAddedEntries)) {
        _SendPrimsAdded(entries);
    }

    _SendPrimsAdded(extraAddedEntries);
    _SendPrimsRemoved(extraRemovedEntries);
    _SendPrimsDirtied(extraDirtyEntries);
}

void MayaUsdRenderPassSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase& /* sender */,
    const HdSceneIndexObserver::RemovedPrimEntries& entries)
{
    HdSceneIndexObserver::AddedPrimEntries   extraAddedEntries;
    HdSceneIndexObserver::DirtiedPrimEntries extraDirtyEntries;
    HdSceneIndexObserver::RemovedPrimEntries extraRemovedEntries;

    if (_EntryCouldAffectPass(entries, _activeRenderPass.renderPassPath)) {
        _UpdateActiveRenderPassState(&extraAddedEntries, &extraDirtyEntries, &extraRemovedEntries);
    }

    if (!_PruneEntries(_activeRenderPass.pruneEval, entries, &extraRemovedEntries)) {
        _SendPrimsRemoved(entries);
    }

    _SendPrimsAdded(extraAddedEntries);
    _SendPrimsRemoved(extraRemovedEntries);
    _SendPrimsDirtied(extraDirtyEntries);
}

void MayaUsdRenderPassSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase& /* sender */,
    const HdSceneIndexObserver::DirtiedPrimEntries& entries)
{
    HdSceneIndexObserver::AddedPrimEntries   extraAddedEntries;
    HdSceneIndexObserver::DirtiedPrimEntries extraDirtyEntries;
    HdSceneIndexObserver::RemovedPrimEntries extraRemovedEntries;

    if (_EntryCouldAffectPass(entries, _activeRenderPass.renderPassPath)) {
        _UpdateActiveRenderPassState(&extraAddedEntries, &extraDirtyEntries, &extraRemovedEntries);
    }

    if (!_PruneEntries(_activeRenderPass.pruneEval, entries, &extraDirtyEntries)) {
        _SendPrimsDirtied(entries);
    }

    _SendPrimsAdded(extraAddedEntries);
    _SendPrimsRemoved(extraRemovedEntries);
    _SendPrimsDirtied(extraDirtyEntries);
}

// Helper method to compile a collection evaluator.
static void _CompileCollection(
    HdCollectionsSchema&                            collections,
    const TfToken&                                  collectionName,
    const HdSceneIndexBaseRefPtr&                   sceneIndex,
    SdfPathExpression*                              expr,
    std::optional<HdCollectionExpressionEvaluator>* eval)
{
    if (HdCollectionSchema collection = collections.GetCollection(collectionName)) {
        if (HdPathExpressionDataSourceHandle pathExprDs = collection.GetMembershipExpression()) {
            *expr = pathExprDs->GetTypedValue(0.0);
            if (!expr->IsEmpty()) {
                *eval = HdCollectionExpressionEvaluator(sceneIndex, *expr);
            }
        }
    }
}

void MayaUsdRenderPassSceneIndex::_UpdateActiveRenderPassState(
    HdSceneIndexObserver::AddedPrimEntries*   addedEntries,
    HdSceneIndexObserver::DirtiedPrimEntries* dirtyEntries,
    HdSceneIndexObserver::RemovedPrimEntries* removedEntries)
{
    TRACE_FUNCTION();

    // Swap out the prior pass state to compare against.
    _RenderPassState& state = _activeRenderPass;
    _RenderPassState  priorState;
    std::swap(state, priorState);

    HdSceneIndexBaseRefPtr inputSceneIndex = _GetInputSceneIndex();
    HdSceneGlobalsSchema   globals = HdSceneGlobalsSchema::GetFromSceneIndex(inputSceneIndex);
    if (HdPathDataSourceHandle pathDs = globals.GetActiveRenderPassPrim()) {
        state.renderPassPath = pathDs->GetTypedValue(0.0);
    }
    if (state.renderPassPath.IsEmpty() && priorState.renderPassPath.IsEmpty()) {
        // Avoid further work if no render pass was or is active.
        return;
    }
    TF_DEBUG(HDVP2_DEBUG_RENDER_PASS)
        .Msg(
            "Filter: active pass <%s> (was <%s>)\n",
            state.renderPassPath.GetText(),
            priorState.renderPassPath.GetText());

    if (!state.renderPassPath.IsEmpty()) {
        const HdSceneIndexPrim passPrim = inputSceneIndex->GetPrim(state.renderPassPath);
        if (!passPrim.dataSource) {
            TF_DEBUG(HDVP2_DEBUG_RENDER_PASS)
                .Msg("  pass prim <%s> not found in input scene\n", state.renderPassPath.GetText());
        }
        if (HdCollectionsSchema collections
            = HdCollectionsSchema::GetFromParent(passPrim.dataSource)) {
            _CompileCollection(
                collections,
                _tokens->renderVisibility,
                inputSceneIndex,
                &state.renderVisExpr,
                &state.renderVisEval);
            _CompileCollection(
                collections, _tokens->prune, inputSceneIndex, &state.pruneExpr, &state.pruneEval);
        }
    }

    const bool visExprDidChange = state.renderVisExpr != priorState.renderVisExpr;

    TF_DEBUG(HDVP2_DEBUG_RENDER_PASS)
        .Msg(
            "  prune '%s', renderVisibility '%s'\n",
            state.pruneExpr.GetText().c_str(),
            state.renderVisExpr.GetText().c_str());

    if (state.pruneExpr == priorState.pruneExpr && !visExprDidChange) {
        // No patterns changed; nothing to invalidate.
        TF_DEBUG(HDVP2_DEBUG_RENDER_PASS).Msg("  no expression changed; nothing to invalidate\n");
        return;
    }

    // Generate change entries for affected prims, considering all upstream prims.
    size_t visited = 0;
    for (const SdfPath& path : HdSceneIndexPrimView(_GetInputSceneIndex())) {
        if (priorState.DoesPrune(path)) {
            if (!state.DoesPrune(path)) {
                // No longer pruned, so add it back.
                HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(path);
                addedEntries->push_back({ path, prim.primType });
            }
        } else if (state.DoesPrune(path)) {
            // Newly pruned, so remove it.
            removedEntries->push_back({ path });
        } else if (visExprDidChange) {
            const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(path);
            if (priorState.DoesOverrideVis(path, prim) != state.DoesOverrideVis(path, prim)) {
                dirtyEntries->push_back({ path, HdVisibilitySchema::GetDefaultLocator() });
            }
        }
        ++visited;
    }

    TF_DEBUG(HDVP2_DEBUG_RENDER_PASS)
        .Msg(
            "  visited %zu prims: %zu removed (pruned), %zu re-added, %zu visibility-dirtied\n",
            visited,
            removedEntries->size(),
            addedEntries->size(),
            dirtyEntries->size());
}

PXR_NAMESPACE_CLOSE_SCOPE
