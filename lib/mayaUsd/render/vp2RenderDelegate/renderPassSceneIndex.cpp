//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Derived from the hdPrman render pass scene index. See the header for why this
// file is kept renderer-neutral.
//
#include "renderPassSceneIndex.h"

// The one non-neutral dependency. On extraction, swap for the hosting library's
// own debug code; everything else here is pure Hydra.
#include "debugCodes.h"

#include <pxr/base/trace/trace.h>
#include <pxr/imaging/hd/collectionSchema.h>
#include <pxr/imaging/hd/collectionsSchema.h>
#include <pxr/imaging/hd/materialBindingsSchema.h>
#include <pxr/imaging/hd/overlayContainerDataSource.h>
#include <pxr/imaging/hd/primvarsSchema.h>
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
    (matte)
    (cameraVisibility)
);
// clang-format on

namespace {

bool _IsGeometryType(const TfToken& primType)
{
    // Gprim types beyond those covered by HdPrimTypeIsGprim().
    static const TfTokenVector extraGeomTypes
        = { HdPrimTypeTokens->cone, HdPrimTypeTokens->cylinder, HdPrimTypeTokens->sphere };
    return HdPrimTypeIsGprim(primType)
        || std::find(extraGeomTypes.begin(), extraGeomTypes.end(), primType) != extraGeomTypes.end();
}

// The prim types the visibility collections apply to.
bool _ShouldApplyPassVisibility(const TfToken& primType)
{
    return _IsGeometryType(primType) || HdPrimTypeIsLight(primType)
        || primType == HdPrimTypeTokens->lightFilter;
}

bool _IsVisible(const HdContainerDataSourceHandle& primSource)
{
    if (const HdVisibilitySchema visSchema = HdVisibilitySchema::GetFromParent(primSource)) {
        if (const HdBoolDataSourceHandle visDs = visSchema.GetVisibility()) {
            return visDs->GetTypedValue(0.0f);
        }
    }
    return true;
}

HdContainerDataSourceHandle
_MakeFlagPrimvar(const MayaUsdRenderPassSceneIndex::FlagPrimvar& flag)
{
    if (flag.name.IsEmpty()) {
        return nullptr;
    }
    return HdRetainedContainerDataSource::New(
        HdPrimvarsSchema::GetSchemaToken(),
        HdRetainedContainerDataSource::New(
            flag.name,
            HdPrimvarSchema::Builder()
                .SetPrimvarValue(HdRetainedTypedSampledDataSource<bool>::New(flag.value))
                .SetInterpolation(
                    HdPrimvarSchema::BuildInterpolationDataSource(HdPrimvarSchemaTokens->constant))
                .Build()));
}

void _CompileCollection(
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

// Scan an entry vector for anything that could affect the active pass.
template <typename ENTRIES>
bool _EntryCouldAffectPass(const ENTRIES& entries, const SdfPath& activeRenderPassPath)
{
    for (const auto& entry : entries) {
        // The prim at the root path holds the HdSceneGlobalsSchema; the prim at
        // the render pass path controls its behaviour.
        if (entry.primPath.IsAbsoluteRootPath() || entry.primPath == activeRenderPassPath) {
            return true;
        }
    }
    return false;
}

// Returns true if any pruning was applied, putting survivors in *postPruneEntries.
template <typename ENTRIES>
bool _PruneEntries(
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

// Deliberately a superset rather than a per-collection setting. Renderers gate
// their work on different dirty bits -- VP2 only reconsiders a shader on
// DirtyMaterialId and only re-evaluates render item enablement on
// DirtyVisibility -- and under-invalidating has been the recurring failure here,
// always showing up when *clearing* a collection rather than applying it.
// Over-invalidating only costs a redundant re-fetch on a user-driven pass switch.
HdDataSourceLocatorSet _FlagDirtyLocators()
{
    HdDataSourceLocatorSet locators;
    locators.insert(HdPrimvarsSchema::GetDefaultLocator());
    locators.insert(HdMaterialBindingsSchema::GetDefaultLocator());
    locators.insert(HdVisibilitySchema::GetDefaultLocator());
    return locators;
}

} // namespace

/* static */
MayaUsdRenderPassSceneIndexRefPtr MayaUsdRenderPassSceneIndex::New(
    const HdSceneIndexBaseRefPtr& inputSceneIndex,
    const Config&                 config)
{
    return TfCreateRefPtr(new MayaUsdRenderPassSceneIndex(inputSceneIndex, config));
}

MayaUsdRenderPassSceneIndex::MayaUsdRenderPassSceneIndex(
    const HdSceneIndexBaseRefPtr& inputSceneIndex,
    const Config&                 config)
    : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
    , _config(config)
    , _matteDs(_MakeFlagPrimvar(config.matte))
    , _cameraInvisibleDs(_MakeFlagPrimvar(config.cameraVisibility))
{
    SetDisplayName("MayaUsd: render passes");
}

MayaUsdRenderPassSceneIndex::~MayaUsdRenderPassSceneIndex() = default;

bool MayaUsdRenderPassSceneIndex::_RenderPassState::DoesPrune(const SdfPath& primPath) const
{
    return pruneEval && pruneEval->Match(primPath);
}

bool MayaUsdRenderPassSceneIndex::_RenderPassState::DoesOverrideVis(
    const SdfPath&          primPath,
    const HdSceneIndexPrim& prim) const
{
    return renderVisEval && _ShouldApplyPassVisibility(prim.primType)
        && !renderVisEval->Match(primPath) && _IsVisible(prim.dataSource);
}

bool MayaUsdRenderPassSceneIndex::_RenderPassState::DoesOverrideMatte(
    const SdfPath&          primPath,
    const HdSceneIndexPrim& prim) const
{
    return matteEval && _IsGeometryType(prim.primType) && matteEval->Match(primPath);
}

bool MayaUsdRenderPassSceneIndex::_RenderPassState::DoesOverrideCameraVis(
    const SdfPath&          primPath,
    const HdSceneIndexPrim& prim) const
{
    // Arnold additionally skips prims already flagged camera-invisible, since it
    // overrides an existing parameter. There is no such prior state here.
    return cameraVisEval && _ShouldApplyPassVisibility(prim.primType)
        && !cameraVisEval->Match(primPath);
}

HdSceneIndexPrim MayaUsdRenderPassSceneIndex::GetPrim(const SdfPath& primPath) const
{
    // Pruning is also applied in GetChildPrimPaths(); doing it here as well keeps
    // a prim pruned even if a downstream scene index asks for it by path.
    if (_activeRenderPass.DoesPrune(primPath)) {
        return HdSceneIndexPrim();
    }

    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    // Renderable prims visible upstream but outside the renderVisibility
    // collection have their visibility overridden to 0.
    if (_activeRenderPass.DoesOverrideVis(primPath, prim)) {
        static const HdContainerDataSourceHandle invisDs = HdRetainedContainerDataSource::New(
            HdVisibilitySchema::GetSchemaToken(),
            HdVisibilitySchema::Builder()
                .SetVisibility(HdRetainedTypedSampledDataSource<bool>::New(false))
                .Build());
        prim.dataSource = HdOverlayContainerDataSource::New(invisDs, prim.dataSource);
    }

    if (_matteDs && _activeRenderPass.DoesOverrideMatte(primPath, prim)) {
        prim.dataSource = HdOverlayContainerDataSource::New(_matteDs, prim.dataSource);
    }

    // Deliberately not HdVisibilitySchema: Hydra visibility gates every render
    // item on the rprim, which for VP2 would disable the shadow-casting item
    // along with the beauty one.
    if (_cameraInvisibleDs && _activeRenderPass.DoesOverrideCameraVis(primPath, prim)) {
        prim.dataSource = HdOverlayContainerDataSource::New(_cameraInvisibleDs, prim.dataSource);
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
                collections, _tokens->prune, inputSceneIndex, &state.pruneExpr, &state.pruneEval);
            _CompileCollection(
                collections,
                _tokens->renderVisibility,
                inputSceneIndex,
                &state.renderVisExpr,
                &state.renderVisEval);
            if (_matteDs) {
                _CompileCollection(
                    collections,
                    _tokens->matte,
                    inputSceneIndex,
                    &state.matteExpr,
                    &state.matteEval);
            }
            if (_cameraInvisibleDs) {
                _CompileCollection(
                    collections,
                    _tokens->cameraVisibility,
                    inputSceneIndex,
                    &state.cameraVisExpr,
                    &state.cameraVisEval);
            }
        }
    }

    const bool visExprDidChange = state.renderVisExpr != priorState.renderVisExpr;
    const bool matteExprDidChange = state.matteExpr != priorState.matteExpr;
    const bool cameraVisExprDidChange = state.cameraVisExpr != priorState.cameraVisExpr;
    const bool perPrimExprDidChange
        = visExprDidChange || matteExprDidChange || cameraVisExprDidChange;

    TF_DEBUG(HDVP2_DEBUG_RENDER_PASS)
        .Msg(
            "  prune '%s', renderVisibility '%s', matte '%s', cameraVisibility '%s'\n",
            state.pruneExpr.GetText().c_str(),
            state.renderVisExpr.GetText().c_str(),
            state.matteExpr.GetText().c_str(),
            state.cameraVisExpr.GetText().c_str());

    if (state.pruneExpr == priorState.pruneExpr && !perPrimExprDidChange) {
        // No patterns changed; nothing to invalidate.
        TF_DEBUG(HDVP2_DEBUG_RENDER_PASS).Msg("  no expression changed; nothing to invalidate\n");
        return;
    }

    static const HdDataSourceLocatorSet flagLocators = _FlagDirtyLocators();

    // Generate change entries for affected prims, considering all upstream prims.
    size_t visited = 0;
    for (const SdfPath& path : HdSceneIndexPrimView(_GetInputSceneIndex())) {
        ++visited;
        if (priorState.DoesPrune(path)) {
            if (!state.DoesPrune(path)) {
                // No longer pruned, so add it back.
                HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(path);
                addedEntries->push_back({ path, prim.primType });
            }
        } else if (state.DoesPrune(path)) {
            // Newly pruned, so remove it.
            removedEntries->push_back({ path });
        } else if (perPrimExprDidChange) {
            const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(path);
            HdDataSourceLocatorSet locators;

            if (visExprDidChange
                && priorState.DoesOverrideVis(path, prim) != state.DoesOverrideVis(path, prim)) {
                locators.insert(HdVisibilitySchema::GetDefaultLocator());
            }
            if (matteExprDidChange
                && priorState.DoesOverrideMatte(path, prim)
                    != state.DoesOverrideMatte(path, prim)) {
                locators.insert(flagLocators);
            }
            if (cameraVisExprDidChange
                && priorState.DoesOverrideCameraVis(path, prim)
                    != state.DoesOverrideCameraVis(path, prim)) {
                locators.insert(flagLocators);
            }

            if (!locators.IsEmpty()) {
                dirtyEntries->push_back({ path, locators });
            }
        }
    }

    TF_DEBUG(HDVP2_DEBUG_RENDER_PASS)
        .Msg(
            "  visited %zu prims: %zu removed (pruned), %zu re-added, %zu dirtied\n",
            visited,
            removedEntries->size(),
            addedEntries->size(),
            dirtyEntries->size());
}

PXR_NAMESPACE_CLOSE_SCOPE
