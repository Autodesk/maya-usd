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
#include "sceneIndexProducer.h"

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/getenv.h>
#include <pxr/imaging/hd/dependencyForwardingSceneIndex.h>
#include <pxr/imaging/hd/dirtyBitsTranslator.h>
#include <pxr/imaging/hd/materialBindingsSchema.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/retainedDataSource.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hdsi/implicitSurfaceSceneIndex.h>
#include <pxr/imaging/hdsi/materialBindingResolvingSceneIndex.h>
#include <pxr/imaging/hdsi/nurbsApproximatingSceneIndex.h>
#include <pxr/imaging/hdsi/unboundMaterialPruningSceneIndex.h>
#include <pxr/imaging/hdx/pickTask.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usdImaging/usdImaging/sceneIndices.h>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

/*! \brief  Configures HdsiImplicitSurfaceSceneIndex to generate a mesh for
            every implicit primitive.

    Mirrors HdSt_ImplicitSurfaceSceneIndexPlugin. The alternative
    axisToTransform mode only suits renderers that draw cones, capsules and
    cylinders natively; HdVP2RenderDelegate::GetSupportedRprimTypes is limited
    to basisCurves, mesh and points, so everything has to become a mesh.
*/
HdContainerDataSourceHandle _ImplicitSurfaceArgs()
{
    static const HdDataSourceBaseHandle toMesh
        = HdRetainedTypedSampledDataSource<TfToken>::New(
            HdsiImplicitSurfaceSceneIndexTokens->toMesh);

    static const HdContainerDataSourceHandle args = HdRetainedContainerDataSource::New(
        HdPrimTypeTokens->sphere,
        toMesh,
        HdPrimTypeTokens->cube,
        toMesh,
        HdPrimTypeTokens->cone,
        toMesh,
        HdPrimTypeTokens->cylinder,
        toMesh,
        HdPrimTypeTokens->capsule,
        toMesh,
        HdPrimTypeTokens->plane,
        toMesh);

    return args;
}

/*! \brief  Escape hatch for the material pruning pair.

    Mirrors Storm's HDST_ENABLE_UNBOUND_MATERIAL_PRUNING_SCENE_INDEX, which is
    also on by default, using the HDVP2_ TfGetenvInt convention the rest of this
    render delegate uses. Set to 0 if a scene loses materials. One known cause:
    HdsiUnboundMaterialPruningSceneIndex forgets that a material is bound when
    the material prim itself is removed, and on re-add only counts bindings from
    gprims in that same batch. So a material deleted and then restored, by an
    undo for instance, stays pruned until a gprim binding it is re-added or has
    its binding dirtied.
*/
bool _IsUnboundMaterialPruningEnabled()
{
    static const bool enabled = TfGetenvInt("HDVP2_ENABLE_UNBOUND_MATERIAL_PRUNING", 1) != 0;
    return enabled;
}

//! Matches the range UsdImagingDelegate accepts.
constexpr int kMaxRefineLevel = 8;

/*! \brief  Maps a VP2 instance index onto one HdxPrimOriginInfo can consume.

    VP2 passes UsdImagingDelegate::ALL_INSTANCES (-1) whenever it wants the prim
    rather than a specific instance - MayaUsdRPrim's constructor and
    _SyncDisplayLayerModes both do. HdxPrimOriginInfo has no equivalent: it walks
    the instancers taking 'instanceIndex % n' at each level, and because n is a
    size_t a negative index converts to a huge value and silently picks an
    arbitrary instance rather than failing.

    An instanced prim has no single scene path to report, so resolve as instance
    0: it is the only instance a single-instance pick (drawInstID <= 0) can
    mean, and it gives the MayaUsdRPrim constructor a stage path to tag with. A
    non-instanced prim ignores the index.
*/
int _ResolveInstanceIndex(int instanceIndex) { return instanceIndex < 0 ? 0 : instanceIndex; }

} // namespace

HdVP2SceneIndexProducer::~HdVP2SceneIndexProducer()
{
    // ProxyRenderDelegate destroys the producer before the render index, both in
    // _ClearRenderDelegate and by member declaration order, so the render index
    // is still alive here. Removing the chain sends PrimsRemoved through
    // emulation, which drops our rprims - the same effect ~UsdImagingDelegate
    // has through RemoveSubtree.
    if (_dirtying) {
        _renderIndex->RemoveSceneIndex(_dirtying);
    }
}

void HdVP2SceneIndexProducer::Initialize(HdRenderIndex* renderIndex, const SdfPath& producerId)
{
    _renderIndex = renderIndex;
    _producerId = producerId;

    // Mirrors UsdImagingGLEngine::_CreateUsdImagingSceneIndices and
    // _AppendOverridesSceneIndices. The chain built by
    // UsdImagingCreateSceneIndices contains no subtree scoping, exclusion
    // pruning, root overrides or display style scene index: the first three are
    // installed through overridesSceneIndexCallback, the last goes on top of the
    // terminal index.
    UsdImagingCreateSceneIndicesInfo info;
    info.displayUnloadedPrimsWithBounds = false;
    info.overridesSceneIndexCallback
        = [this](const HdSceneIndexBaseRefPtr& inputScene) -> HdSceneIndexBaseRefPtr {
        // Both filters work on stage paths and ahead of instancing, as in
        // UsdImagingGLEngine: applied any later, a native instance outside the
        // scope, or excluded, would still be drawn by the instancer the NI
        // prototype propagating scene index aggregates it into. It also keeps
        // filtered gprims out of material binding resolution, so materials only
        // they bind are pruned too. Neither renames a prim, so ToIndexPath is
        // unaffected. The root path and exclusions arrive in Populate; until
        // then neither filters anything.
        _subtreeScoping = HdVP2SubtreeScopingSceneIndex::New(inputScene);
        _exclusionPruning = HdsiPrefixPathPruningSceneIndex::New(_subtreeScoping, nullptr);
        _rootOverrides = UsdImagingRootOverridesSceneIndex::New(_exclusionPruning);
        return _rootOverrides;
    };

    const UsdImagingSceneIndices sceneIndices = UsdImagingCreateSceneIndices(info);
    _stageSceneIndex = sceneIndices.stageSceneIndex;
    _noticeBatchingSceneIndex = sceneIndices.postInstancingNoticeBatchingSceneIndex;
    _selectionSceneIndex = sceneIndices.selectionSceneIndex;

    // Observing the selection scene index directly, rather than the chain
    // terminal, keeps the paths it reports chain-local, which is what
    // ToIndexPath expects.
    //
    // Attaching before the stage is set only makes the traversal SetSceneIndex
    // performs trivial. The observer still queues every prim SetStage adds,
    // and every prim a later resync re-adds, and the next GetSelection queries
    // each of them. See PopulateSelection.
    _selectionObserver.SetSceneIndex(_selectionSceneIndex);

    // Both of the scene indices below, like the implicit surface one further
    // down, are ones Storm receives from the renderer plugin chain. VP2 gets
    // none of that chain (see the comment on the implicit surface scene index),
    // so they are inserted by hand, in the order Storm registers them:
    // HdSt_MaterialBindingResolvingSceneIndexPlugin at insertion phase 0 and
    // HdSt_UnboundMaterialPruningSceneIndexPlugin at phase 900.
    HdSceneIndexBaseRefPtr materials = sceneIndices.finalSceneIndex;

    if (_IsUnboundMaterialPruningEnabled()) {
        // The material binding purposes VP2 renders, in priority order: the
        // render delegate's, then allPurpose, the fallback UsdShade applies.
        //
        // UsdImagingDelegate resolved a gprim's binding for the render
        // delegate's GetMaterialBindingPurpose and populated only the material
        // that won, so a material bound solely under another purpose never
        // became an sprim. UsdImagingStageSceneIndex has no such notion: every
        // UsdShadeMaterial prim on the stage becomes an hdMaterial prim, the
        // render index turns each into an sprim and SyncAll syncs them all. On
        // ALab that is 1110 usd_full materials nothing draws, whose UDIM
        // textures cost 387 of the 409 seconds spent loading textures.
        const TfTokenVector purposes {
            renderIndex->GetRenderDelegate()->GetMaterialBindingPurpose(),
            HdMaterialBindingsSchemaTokens->allPurpose
        };

        // The resolver has to come first. The pruner treats its purpose list as
        // a union - a material is bound if any listed purpose points at it -
        // whereas UsdImagingDelegate resolved by priority. A gprim binding
        // usd_full under allPurpose and usd_preview under preview would keep
        // both. Collapsing the bindings first leaves only the priority winner
        // visible, so the pruner sees what the legacy producer would have.
        //
        // Safe for bound materials: the emulation delegate's GetMaterialId asks
        // for GetMaterialBindingPurpose, and
        // HdMaterialBindingsSchema::GetMaterialBinding falls back to allPurpose
        // when that purpose is absent. Storm relies on the same fallback.
        materials = HdsiMaterialBindingResolvingSceneIndex::New(
            materials, purposes, HdMaterialBindingsSchemaTokens->allPurpose);

        // Clears the prim type and data source of every material no gprim
        // binds, which stops the render index creating an sprim for it.
        materials = HdsiUnboundMaterialPruningSceneIndex::New(
            materials,
            HdRetainedContainerDataSource::New(
                HdsiUnboundMaterialPruningSceneIndexTokens->materialBindingPurposes,
                HdRetainedTypedSampledDataSource<VtArray<TfToken>>::New(
                    VtArray<TfToken>(purposes.begin(), purposes.end()))));
    }

    // VP2 draws only basisCurves, mesh and points, so implicit surfaces have to
    // be turned into meshes. Storm gets this from
    // HdSt_ImplicitSurfaceSceneIndexPlugin, which HdRenderIndex appends on its
    // behalf because HdSceneIndexPluginRegistry is keyed on the render
    // delegate's display name. HdVP2RenderDelegate is constructed directly
    // rather than through HdRendererPluginRegistry, so that name is empty and
    // the renderer plugin chain is skipped entirely. Inserting the scene index
    // here puts the conversion at the same point in the chain. Its points
    // invalidation relies on the dependency forwarding scene index below.
    const HdSceneIndexBaseRefPtr implicitSurfaces
        = HdsiImplicitSurfaceSceneIndex::New(materials, _ImplicitSurfaceArgs());

    // For the same reason, NurbsCurves and NurbsPatch would otherwise reach the
    // render index as nurbsCurves and nurbsPatch, which VP2 has no rprims for,
    // and be dropped. UsdImagingDelegate drew them as linear basisCurves and
    // meshes; this does the same, as HdSt_NurbsApproximatingSceneIndexPlugin
    // does for Storm.
    const HdSceneIndexBaseRefPtr nurbs = HdsiNurbsApproximatingSceneIndex::New(implicitSurfaces);

    _displayStyle = HdsiLegacyDisplayStyleOverrideSceneIndex::New(nurbs);

    // The implicit surface and NURBS scene indices above, and the UsdSkelImaging
    // ones UsdImagingCreateSceneIndices appends, state what their generated data
    // depends on only as __dependencies: a capsule's height edit arrives dirtying
    // capsule/height, which means nothing for the mesh it became. This scene
    // index turns those declarations into the derived invalidation - here
    // primvars/points - so the emulation delegate can map it to dirty bits. Storm
    // gets it from HdSt_DependencyForwardingSceneIndexPlugin, last in its chain
    // at phase 1000; it is placed last here for the same reason, so it sees every
    // declaration. It stays below _dirtying, whose marks already name the exact
    // rprim and locators and need no forwarding.
    const HdSceneIndexBaseRefPtr dependencies
        = HdDependencyForwardingSceneIndex::New(_displayStyle);

    // Terminal, so that a PrimsDirtied it emits reaches every downstream
    // observer including the back-end emulation delegate.
    _dirtying = HdVP2DirtyingSceneIndex::New(dependencies);

    // needsPrefixing defaults to true, so the chain is wrapped in an
    // HdPrefixingSceneIndex rooted at producerId. ToIndexPath must agree with it.
    renderIndex->InsertSceneIndex(_dirtying, producerId);
}

bool HdVP2SceneIndexProducer::Populate(
    const UsdStageRefPtr& stage,
    const SdfPath&        rootPath,
    const SdfPathVector&  excludedPaths)
{
    if (!TF_VERIFY(stage, "Cannot populate the VP2 scene index producer without a stage")) {
        return false;
    }

    // ProxyRenderDelegate::_Populate guards on _isPopulated, so these warnings
    // fire once per stage load rather than once per frame.
    if (rootPath.IsEmpty()) {
        // A malformed primPath attribute. Match UsdImagingDelegate, which
        // populates nothing for an invalid root prim, but report success, so
        // the caller latches instead of retrying every frame.
        TF_WARN("The proxy shape's primPath is not a valid prim path; nothing will be drawn.");
        return true;
    }

    const UsdPrim rootPrim = stage->GetPrimAtPath(rootPath);
    if (!rootPrim) {
        // Unlike UsdImagingDelegate, the scope picks the prim up if it is
        // created later.
        TF_WARN(
            "The proxy shape's primPath <%s> does not name a valid prim; nothing will be "
            "drawn until it exists.",
            rootPath.GetText());
    } else if (rootPrim.IsInstanceProxy()) {
        // The stage scene index holds nothing below a native instance, so the
        // scope cannot reach into one; UsdImagingDelegate could.
        TF_WARN(
            "The proxy shape's primPath <%s> is inside a native instance; the whole instance "
            "is drawn, offset by the primPath's transform within it.",
            rootPath.GetText());
    }

    // UsdImagingDelegate never visited the root prim's ancestors, so it ignored
    // an exclusion of one of them, where the pruner would remove the whole
    // scope. Excluding the root prim itself still draws nothing, as it did.
    SdfPathVector effectiveExcludedPaths;
    effectiveExcludedPaths.reserve(excludedPaths.size());
    for (const SdfPath& excludedPath : excludedPaths) {
        if (excludedPath == rootPath || !rootPath.HasPrefix(excludedPath)) {
            effectiveExcludedPaths.push_back(excludedPath);
        }
    }

    // SetStage populates internally, and ending the batch flushes it, so the
    // rprims exist when Populate returns.
    _noticeBatchingSceneIndex->SetBatchingEnabled(true);

    // Both before SetStage, so prims out of scope or excluded are never added in
    // the first place. SetRootPath sends no notices, so it relies on running on
    // a fresh chain: ProxyRenderDelegate rebuilds the producer whenever primPath
    // or excludePrimPaths changes (_ClearInvalidData).
    _subtreeScoping->SetRootPath(rootPath);
    _exclusionPruning->SetExcludePathPrefixes(effectiveExcludedPaths);

    _stageSceneIndex->SetStage(stage);
    _noticeBatchingSceneIndex->SetBatchingEnabled(false);

    return true;
}

void HdVP2SceneIndexProducer::ApplyPendingUpdates()
{
    // Batched like the initial SetStage, as UsdImagingGLEngine does.
    _noticeBatchingSceneIndex->SetBatchingEnabled(true);
    _stageSceneIndex->ApplyPendingUpdates();
    _noticeBatchingSceneIndex->SetBatchingEnabled(false);
}

void HdVP2SceneIndexProducer::SetTime(const UsdTimeCode& timeCode)
{
    _stageSceneIndex->SetTime(timeCode);
}

UsdTimeCode HdVP2SceneIndexProducer::GetTime() const { return _stageSceneIndex->GetTime(); }

void HdVP2SceneIndexProducer::SetRootTransform(const GfMatrix4d& transform)
{
    _rootOverrides->SetRootTransform(transform);
}

GfMatrix4d HdVP2SceneIndexProducer::GetRootTransform() const
{
    return _rootOverrides->GetRootTransform();
}

void HdVP2SceneIndexProducer::SetRootVisibility(bool visible)
{
    _rootOverrides->SetRootVisibility(visible);
}

bool HdVP2SceneIndexProducer::GetRootVisibility() const
{
    return _rootOverrides->GetRootVisibility();
}

void HdVP2SceneIndexProducer::SetRefineLevelFallback(int level)
{
    if (level < 0 || level > kMaxRefineLevel) {
        TF_CODING_ERROR(
            "Invalid refinement level(%d), expected range is [0,%d].", level, kMaxRefineLevel);
        return;
    }

    _refineLevelFallback = level;

    // UsdImagingStageSceneIndex has no complexity opinion, so this supplies the
    // fallback for every prim.
    _displayStyle->SetRefineLevelFallback(level);
}

int HdVP2SceneIndexProducer::GetRefineLevelFallback() const { return _refineLevelFallback; }

SdfPath HdVP2SceneIndexProducer::ToIndexPath(const SdfPath& usdPath) const
{
    // The call HdPrefixingSceneIndex::AddPrefix makes, so it matches the
    // prefixing scene index InsertSceneIndex wrapped the chain in.
    return usdPath.ReplacePrefix(
        SdfPath::AbsoluteRootPath(), _producerId, /* fixTargetPaths = */ false);
}

/*  The exact inverse of ToIndexPath: the HdPrefixingSceneIndex that
    InsertSceneIndex wrapped the chain in is the only thing standing between a
    chain path and an rprim id.

    Deliberately separate from GetScenePrimPath, which answers a different
    question - where on the USD stage a prim came from. The two agree for
    non-instanced prims, which is why they used to be one function, but
    instancing pulls them apart. Callers that need a path they can hand back to
    the chain, rather than to the stage, want this one.
*/
SdfPath HdVP2SceneIndexProducer::_ToChainPath(const SdfPath& indexPath) const
{
    // As in HdPrefixingSceneIndex::GetPrim, only paths under producerId map
    // into the chain.
    return indexPath.HasPrefix(_producerId)
        ? indexPath.ReplacePrefix(
              _producerId, SdfPath::AbsoluteRootPath(), /* fixTargetPaths = */ false)
        : SdfPath();
}

/*  Resolves an rprim, and optionally one of its instances, back to the USD prim
    it was generated from.

    An instanced prim's rprim does not sit at its USD path. The propagating scene
    indices in UsdImagingCreateSceneIndices move it under a synthesized prototype
    root, and a single rprim then stands in for every instance, so stripping
    producerId no longer produces a path that exists on the stage. The scene path
    has to be reassembled from the primOrigin data sources of the prim itself and
    of each instancer above it, which is what HdxPrimOriginInfo does. It reads
    the chain at _displayStyle, below the dependency forwarding scene index,
    whose GetPrim records dependencies HdxPrimOriginInfo never reads.

    This is not only a picking entry point. MayaUsdRPrim's constructor uses it to
    build the _PrimSegmentString that tags every MRenderItem with its UFE
    identifier, and _SyncDisplayLayerModes walks the returned path's ancestors to
    resolve display layer membership. Returning an empty path breaks drawing, not
    just selection.
*/
SdfPath HdVP2SceneIndexProducer::GetScenePrimPath(
    const SdfPath&      rprimId,
    int                 instanceIndex,
    HdInstancerContext* instancerContext) const
{
    const SdfPath chainPath = _ToChainPath(rprimId);
    if (chainPath.IsEmpty()) {
        return chainPath;
    }

    HdxPickHit hit {};
    hit.objectId = chainPath;
    hit.instanceIndex = _ResolveInstanceIndex(instanceIndex);

    const HdxPrimOriginInfo info = HdxPrimOriginInfo::FromPickHit(_displayStyle, hit);

    if (instancerContext) {
        // Only authored instancers contribute entries, so this stays empty for
        // native instancing - the distinction _SyncDisplayLayerModes and
        // ProxyRenderDelegate::GetPathInPrototype branch on.
        *instancerContext = info.ComputeInstancerContext();
    }

    const SdfPath scenePath = info.GetFullPath();

    // No primOrigin means the prim did not come from the stage - the chain
    // synthesized it. Its chain path is the only answer available.
    return scenePath.IsEmpty() ? chainPath : scenePath;
}

// The result is indexed positionally by the caller, so it must carry one entry
// per requested instance.
//
// FromPickHits rather than a loop over FromPickHit: it caches the instance index
// and location arrays it computes per instancer across the whole batch, and
// every caller here asks about many instances of one prim at once.
SdfPathVector HdVP2SceneIndexProducer::GetScenePrimPaths(
    const SdfPath&          rprimId,
    const std::vector<int>& instanceIndexes) const
{
    const SdfPath chainPath = _ToChainPath(rprimId);
    if (chainPath.IsEmpty()) {
        return SdfPathVector(instanceIndexes.size(), chainPath);
    }

    std::vector<HdxPickHit> hits(instanceIndexes.size());
    for (size_t i = 0; i < instanceIndexes.size(); ++i) {
        hits[i].objectId = chainPath;
        hits[i].instanceIndex = _ResolveInstanceIndex(instanceIndexes[i]);
    }

    const std::vector<HdxPrimOriginInfo> infos
        = HdxPrimOriginInfo::FromPickHits(_displayStyle, hits);

    SdfPathVector scenePaths;
    scenePaths.reserve(infos.size());
    for (const HdxPrimOriginInfo& info : infos) {
        const SdfPath scenePath = info.GetFullPath();
        scenePaths.push_back(scenePath.IsEmpty() ? chainPath : scenePath);
    }

    return scenePaths;
}

/*  Populates the HdSelection object VP2 highlights from, via
    ProxyRenderDelegate::GetSelectionStatus.

    The reverse of GetScenePrimPath, and there is no single call for it. An
    instanced prim has no rprim at its USD path, so the path has to be resolved
    to the prototype prims standing in for it, plus the indices of the instances
    the selection covers. UsdImagingSelectionSceneIndex::AddSelection does that
    resolution and records it as an HdSelectionsSchema on the affected prims;
    HdxSelectionSceneIndexObserver reads those back out as an HdSelection.

    The scene index keeps one flat selection state with no notion of lead versus
    active, and VP2 wants those in separate HdSelection objects, so each path is
    resolved on its own and the state is cleared again afterwards.

    Each GetSelection queries the prims the observer queued since the previous
    call, plus those in its previous result. Selection edits queue only the
    prims whose selection changed, but the observer also queues every prim
    added to the scene. So the first call after SetStage, or after a resync
    that re-adds a subtree, visits every added prim: its cost is proportional to
    the size of the stage. After that, the cost tracks what is selected.

    AddSelection takes only a path, so a single point instance - what the
    "Instances" pick mode produces - highlights every instance of its point
    instancer for now.
*/
void HdVP2SceneIndexProducer::PopulateSelection(
    const SdfPath& usdPath,
    int /* instanceIndex */,
    const HdSelectionSharedPtr& result)
{
    // An expired UFE item has no path. AddSelection would treat the empty path
    // as the root and select the whole stage.
    if (usdPath.IsEmpty()) {
        return;
    }

    _selectionSceneIndex->AddSelection(usdPath);

    if (const HdSelectionSharedPtr resolved = _selectionObserver.GetSelection()) {
        const SdfPathVector chainPaths
            = resolved->GetSelectedPrimPaths(HdSelection::HighlightModeSelect);

        for (const SdfPath& chainPath : chainPaths) {
            const HdSelection::PrimSelectionState* const state
                = resolved->GetPrimSelectionState(HdSelection::HighlightModeSelect, chainPath);
            if (!state) {
                continue;
            }

            // The observer reports chain paths, and VP2 compares these against
            // rprim ids.
            const SdfPath indexPath = ToIndexPath(chainPath);

            if (state->fullySelected) {
                result->AddRprim(HdSelection::HighlightModeSelect, indexPath);
            }

            // Instance ids, nesting flattened outermost-first: the space HdVP2Mesh
            // and HdVP2Instancer index with. USD 26.05's observer computes them
            // without the instancer's mask, so they can run past an rprim's
            // instances; the rprims skip those.
            for (const VtIntArray& instanceIds : state->instanceIndices) {
                result->AddInstance(HdSelection::HighlightModeSelect, indexPath, instanceIds);
            }
        }
    }

    _selectionSceneIndex->ClearSelection();
}

/*  Emits the invalidation from inside our own chain rather than through
    HdChangeTracker, which cannot reach prims contributed by
    HdRenderIndex::InsertSceneIndex - see HdVP2DirtyingSceneIndex.

    Notices carry chain-local paths: HdPrefixingSceneIndex adds producerId on the
    way out, mirroring ToIndexPath. From there the notice reaches
    HdSceneIndexAdapterSceneDelegate, which runs the locators back through
    HdDirtyBitsTranslator and marks the rprim for real. Custom bits survive the
    round trip as __customBits locators.
*/
void HdVP2SceneIndexProducer::MarkRprimDirty(const SdfPath& indexPath, HdDirtyBits bits)
{
    _MarkDirty(indexPath, bits, &HdDirtyBitsTranslator::RprimDirtyBitsToLocatorSet);
}

void HdVP2SceneIndexProducer::MarkSprimDirty(const SdfPath& indexPath, HdDirtyBits bits)
{
    _MarkDirty(indexPath, bits, &HdDirtyBitsTranslator::SprimDirtyBitsToLocatorSet);
}

void HdVP2SceneIndexProducer::_MarkDirty(
    const SdfPath& indexPath,
    HdDirtyBits    bits,
    _ToLocators    toLocators)
{
    if (bits == HdChangeTracker::Clean) {
        return;
    }

    // _ToChainPath, not GetScenePrimPath: the notice has to name a prim the
    // chain holds. An instanced prim's stage path is not one of those, so
    // resolving to it here would drop the invalidation on the floor.
    const SdfPath chainPath = _ToChainPath(indexPath);
    if (chainPath.IsEmpty()) {
        return;
    }

    HdDataSourceLocatorSet locators;
    toLocators(_displayStyle->GetPrim(chainPath).primType, bits, &locators);
    if (locators.IsEmpty()) {
        return;
    }

    // Batching is only switched on here, right before a notice is held, so it
    // being on means something is held. Switching it off flushes that first, so
    // notices leave in the order the marks were made.
    _dirtying->SetBatchingEnabled(_renderIndex->IsSyncAllInProgress());
    _dirtying->DirtyPrims({ { chainPath, locators } });
}

bool HdVP2SceneIndexProducer::FlushDeferredUpdates()
{
    if (!TF_VERIFY(
            !_renderIndex->IsSyncAllInProgress(),
            "Deferred dirty marks cannot be flushed during SyncAll")
        || !_dirtying->IsBatchingEnabled()) {
        return false;
    }

    _dirtying->SetBatchingEnabled(false);
    return true;
}

HdSceneDelegate* HdVP2SceneIndexProducer::GetSceneDelegate() const
{
    // No legacy scene delegate inserted producerId, so this resolves to the
    // render index's back-end emulation delegate, which owns every prim the
    // chain contributes.
    return _renderIndex->GetSceneDelegateForRprim(_producerId);
}

UsdImagingDelegate* HdVP2SceneIndexProducer::GetUsdImagingDelegate() const
{
    // Once per session. A function-local static initializes thread-safely.
    static const bool warned = [] {
        TF_CODING_ERROR(
            "ProxyRenderDelegate::GetUsdImagingDelegate() returns nullptr in a build with "
            "CMAKE_WANT_MAYAUSD_VP2_USE_SCENE_INDEX=ON, because VP2 draws USD through a "
            "Hydra 2.0 scene index instead. Use GetHdSceneDelegate().");
        return true;
    }();
    (void)warned;
    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
