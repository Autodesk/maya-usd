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
#ifndef HD_VP2_IMPLICIT_DOUBLE_SIDED_SCENE_INDEX
#define HD_VP2_IMPLICIT_DOUBLE_SIDED_SCENE_INDEX

#include <pxr/base/tf/notice.h>
#include <pxr/imaging/hd/filteringSceneIndex.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/common.h>
#include <pxr/usd/usd/notice.h>

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdVP2DoubleSidedTaggingSceneIndex);
TF_DECLARE_REF_PTRS(HdVP2DoubleSidedRestoringSceneIndex);

/*! \brief  Tags every implicit surface with its USD doubleSided value, so that the
            value survives the conversion to a mesh. Pairs with
            HdVP2DoubleSidedRestoringSceneIndex.

    UsdImaging publishes no doubleSided for a Cube, Sphere, Cone, Cylinder or
    Capsule: the attribute belongs to UsdGeomGprim and the implicit schemas carry
    none. VP2 draws only meshes, so HdsiImplicitSurfaceSceneIndex converts them,
    and it hard-codes doubleSided to false in every mesh it generates (USD 26.05,
    26.08 and dev). A stage UsdImagingDelegate drew double-sided would draw with
    its back faces culled, and an edit of the attribute would never be seen.

    The plane is converted with the doubleSided its schema carries, but an edit of
    that does not dirty mesh/doubleSided either, so it is tagged as well.

    This scene index goes ahead of instancing, where prims are still at stage
    paths and still have their implicit type:

    - The tag is a data source that reads the attribute from the stage when
      asked. It is part of the prim, so it travels through native and point
      instancing and the conversion, and the restoring scene index finds it on
      the mesh.
    - UsdImaging sends no invalidation when the attribute of an implicit surface
      changes, so this scene index listens to the stage, as
      UsdImagingStageSceneIndex does, and dirties mesh/doubleSided. The notices
      are sent from ApplyPendingUpdates, after the stage scene index has applied
      its own.

    Both scene indices can go once USD publishes doubleSided for implicit surfaces.

    \class  HdVP2DoubleSidedTaggingSceneIndex
*/
class HdVP2DoubleSidedTaggingSceneIndex final : public HdSingleInputFilteringSceneIndexBase
{
public:
    static HdVP2DoubleSidedTaggingSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex);

    ~HdVP2DoubleSidedTaggingSceneIndex() override;

    //! \brief  Sets the stage the attribute is read from and listened to. Call before the
    //!         input is populated.
    void SetStage(const UsdStageRefPtr& stage);

    //! \brief  Sends the dirty notices for the edits made since the last call. Call after
    //!         UsdImagingStageSceneIndex::ApplyPendingUpdates.
    void ApplyPendingUpdates();

    HdSceneIndexPrim GetPrim(const SdfPath& primPath) const override;
    SdfPathVector    GetChildPrimPaths(const SdfPath& primPath) const override;

protected:
    HdVP2DoubleSidedTaggingSceneIndex(const HdSceneIndexBaseRefPtr& inputSceneIndex);

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
    void _OnObjectsChanged(const UsdNotice::ObjectsChanged& notice, const UsdStageWeakPtr& sender);

    UsdStageWeakPtr _stage;
    TfNotice::Key   _noticeKey;

    //! The prims whose doubleSided was edited, collected on whichever thread edited the stage.
    std::mutex _pendingMutex;
    SdfPathSet _pendingPaths;
};

/*! \brief  Publishes the value HdVP2DoubleSidedTaggingSceneIndex tagged a prim with as
            the doubleSided of the mesh it became.

    Goes after HdsiImplicitSurfaceSceneIndex. A mesh that was not an implicit
    surface has no tag and passes through.

    \class  HdVP2DoubleSidedRestoringSceneIndex
*/
class HdVP2DoubleSidedRestoringSceneIndex final : public HdSingleInputFilteringSceneIndexBase
{
public:
    static HdVP2DoubleSidedRestoringSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex);

    HdSceneIndexPrim GetPrim(const SdfPath& primPath) const override;
    SdfPathVector    GetChildPrimPaths(const SdfPath& primPath) const override;

protected:
    HdVP2DoubleSidedRestoringSceneIndex(const HdSceneIndexBaseRefPtr& inputSceneIndex);

    void _PrimsAdded(
        const HdSceneIndexBase&                       sender,
        const HdSceneIndexObserver::AddedPrimEntries& entries) override;

    void _PrimsRemoved(
        const HdSceneIndexBase&                         sender,
        const HdSceneIndexObserver::RemovedPrimEntries& entries) override;

    void _PrimsDirtied(
        const HdSceneIndexBase&                         sender,
        const HdSceneIndexObserver::DirtiedPrimEntries& entries) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
