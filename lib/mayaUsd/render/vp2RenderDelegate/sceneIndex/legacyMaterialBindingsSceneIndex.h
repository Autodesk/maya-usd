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
#ifndef HD_VP2_LEGACY_MATERIAL_BINDINGS_SCENE_INDEX
#define HD_VP2_LEGACY_MATERIAL_BINDINGS_SCENE_INDEX

#include <pxr/base/tf/notice.h>
#include <pxr/imaging/hd/dataSourceLocator.h>
#include <pxr/imaging/hd/filteringSceneIndex.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/common.h>
#include <pxr/usd/usd/notice.h>
#include <pxr/usdImaging/usdImaging/types.h>

#include <map>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdVP2LegacyMaterialBindingsSceneIndex);

/*! \brief  Gives the prims that bind a material without applying MaterialBindingAPI the
            material bindings UsdImagingStageSceneIndex leaves them without.

    UsdImagingDelegate resolved a binding with UsdShadeMaterialBindingAPI::ComputeBoundMaterial,
    whose supportLegacyBindings defaults to true: a material:binding authored on a prim that does
    not apply the schema was honored, as stages written before the schema was an applied one rely
    on. UsdImagingStageSceneIndex gets material bindings only from the API schema adapter, which
    is chosen from the schemas a prim applies, so such a prim has none. A gprim loses the material
    it binds, VP2 draws its fallback shader, and HdsiUnboundMaterialPruningSceneIndex removes a
    material nothing else binds. USD documents the legacy default as going away.

    This scene index goes ahead of instancing, where prims are still at stage paths, and gives a
    prim without materialBindings, but with a property named material:binding..., the data source
    UsdImagingMaterialBindingAPIAdapter returns for a prim that does apply the schema. Native
    instance aggregation, the flattening of bindings down the namespace, the translation of
    instance proxy paths and the resolution to Hydra bindings then treat it as they treat any
    other. A prim that applies the schema keeps the data USD gave it.

    UsdImagingStageSceneIndex sends no invalidation for these prims, because the adapter is not
    registered for them, so this scene index listens to the stage, as UsdImagingStageSceneIndex
    does, and dirties materialBindings with the locators the adapter names. The notices are sent
    from ApplyPendingUpdates, after the stage scene index has applied its own.

    Collection-based bindings also need the collection prim to be published, which takes
    CollectionAPI to be applied to it, and that is not changed here.

    \class  HdVP2LegacyMaterialBindingsSceneIndex
*/
class HdVP2LegacyMaterialBindingsSceneIndex final : public HdSingleInputFilteringSceneIndexBase
{
public:
    static HdVP2LegacyMaterialBindingsSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex);

    ~HdVP2LegacyMaterialBindingsSceneIndex() override;

    //! \brief  Sets the stage the bindings are read from and listened to. Call before the input
    //!         is populated.
    void SetStage(const UsdStageRefPtr& stage);

    //! \brief  Sends the dirty notices for the edits made since the last call. Call after
    //!         UsdImagingStageSceneIndex::ApplyPendingUpdates.
    void ApplyPendingUpdates();

    HdSceneIndexPrim GetPrim(const SdfPath& primPath) const override;
    SdfPathVector    GetChildPrimPaths(const SdfPath& primPath) const override;

protected:
    HdVP2LegacyMaterialBindingsSceneIndex(const HdSceneIndexBaseRefPtr& inputSceneIndex);

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
    //! The properties edited on each prim, as UsdImagingStageSceneIndex keeps them.
    using PropertyMap = std::map<SdfPath, TfTokenVector>;
    //! What to dirty on each prim.
    using LocatorMap = std::map<SdfPath, HdDataSourceLocatorSet>;

    void _OnObjectsChanged(const UsdNotice::ObjectsChanged& notice, const UsdStageWeakPtr& sender);

    //! Adds what the edits of the given kind dirty to the locators of their prims.
    void _Collect(
        const PropertyMap&                 properties,
        UsdImagingPropertyInvalidationType type,
        LocatorMap*                        locators) const;

    UsdStageWeakPtr _stage;
    TfNotice::Key   _noticeKey;

    //! The edits, collected on whichever thread edited the stage.
    std::mutex  _pendingMutex;
    PropertyMap _pendingResynced;
    PropertyMap _pendingUpdated;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
