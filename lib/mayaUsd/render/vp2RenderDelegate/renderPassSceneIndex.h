//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Derived from the hdPrman render pass scene index, by way of arnold-usd's
// plugins/scene_index/renderPassSIP.cpp, which is itself a modified copy of it.
//
// Kept renderer-neutral so it can be lifted into a shared library
// (hydra-viewport-toolbox, or ideally OpenUSD's hdsi alongside the prune-only
// HdsiRenderPassPruneSceneIndex). The only Maya-specific thing left in the .cpp
// is the debugCodes.h include; everything a renderer needs to vary is in Config,
// supplied by the caller.
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

/// Applies the active render pass named in the HdSceneGlobalsSchema.
///
/// UsdRenderPass defines a fixed set of four collections, all handled here:
///
///   prune             prims are removed from the scene
///   renderVisibility  prims outside the collection get visibility=0
///   matte             prims inside the collection are flagged
///   cameraVisibility  prims outside the collection are flagged
///
/// prune and renderVisibility need no renderer-specific interpretation and are
/// always applied. matte and cameraVisibility have no renderer-neutral meaning,
/// so the renderer says which primvar to flag them with; leaving a name empty
/// disables that collection.
///
/// \note Assumes the active render pass is a UsdRenderPass, for collection
///       naming conventions.
class MayaUsdRenderPassSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    /// A constant bool primvar written on affected prims. Constant primvars are
    /// the only channel scene index emulation forwards to a render delegate,
    /// which is why the flags are carried this way.
    struct FlagPrimvar
    {
        /// Empty disables the collection entirely.
        TfToken name;

        /// The value written on affected prims. matte is set true; camera
        /// visibility is set false, since the primvar states whether the prim is
        /// visible to camera rather than whether it is hidden. Both match
        /// Arnold's arnold:matte and arnold:visibility:camera, so only the
        /// namespace differs between renderers.
        bool value = true;
    };

    struct Config
    {
        FlagPrimvar matte;
        FlagPrimvar cameraVisibility;
    };

    static MayaUsdRenderPassSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex, const Config& config);

    HdSceneIndexPrim GetPrim(const SdfPath& primPath) const override;
    SdfPathVector    GetChildPrimPaths(const SdfPath& primPath) const override;

protected:
    MayaUsdRenderPassSceneIndex(const HdSceneIndexBaseRefPtr& inputSceneIndex, const Config&);
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
    /// Expressions are retained so old and new state can be compared.
    struct _RenderPassState
    {
        SdfPath           renderPassPath;
        SdfPathExpression pruneExpr;
        SdfPathExpression renderVisExpr;
        SdfPathExpression matteExpr;
        SdfPathExpression cameraVisExpr;

        std::optional<HdCollectionExpressionEvaluator> pruneEval;
        std::optional<HdCollectionExpressionEvaluator> renderVisEval;
        std::optional<HdCollectionExpressionEvaluator> matteEval;
        std::optional<HdCollectionExpressionEvaluator> cameraVisEval;

        bool DoesPrune(const SdfPath& primPath) const;
        bool DoesOverrideVis(const SdfPath& primPath, const HdSceneIndexPrim& prim) const;
        // Matching the collection is what makes a prim matte, the opposite
        // polarity to the two visibility collections where *not* matching hides.
        bool DoesOverrideMatte(const SdfPath& primPath, const HdSceneIndexPrim& prim) const;
        bool DoesOverrideCameraVis(const SdfPath& primPath, const HdSceneIndexPrim& prim) const;
    };

    void _UpdateActiveRenderPassState(
        HdSceneIndexObserver::AddedPrimEntries*   addedEntries,
        HdSceneIndexObserver::DirtiedPrimEntries* dirtyEntries,
        HdSceneIndexObserver::RemovedPrimEntries* removedEntries);

    const Config                _config;
    HdContainerDataSourceHandle _matteDs;
    HdContainerDataSourceHandle _cameraInvisibleDs;
    _RenderPassState            _activeRenderPass;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAUSD_VP2_RENDER_PASS_SCENE_INDEX_H
