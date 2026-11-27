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
#ifndef HD_VP2_FALLBACK_EXTENT_SCENE_INDEX
#define HD_VP2_FALLBACK_EXTENT_SCENE_INDEX

#include <pxr/imaging/hd/filteringSceneIndex.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/common.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdVP2FallbackExtentSceneIndex);

/*! \brief  Gives the prims UsdImagingStageSceneIndex publishes no extent for the extent
            UsdImagingDelegate gave them.

    UsdImagingStageSceneIndex publishes an extent only where one is authored, or can be resolved
    from an authored extentsHint. Two kinds of prim that UsdImagingDelegate drew with an extent are
    left without:

    - A Cube, Sphere, Cone, Cylinder, Capsule or Plane with no authored extent.
      UsdImagingDelegate read the attribute, which gives the schema's fallback when nothing is
      authored. VP2 draws the bounding box of such a prim from it and has none to draw without.
      The value is the schema's, as it was, and is not computed from the size attributes: a
      sphere of radius 4 gets the box of a unit sphere.

    - A model drawn with a draw mode of cards, bounds or origin, with no authored extent and no
      extentsHint. UsdImagingDrawModeAdapter computed one from the model's children with
      UsdGeomBBoxCache, as UsdImagingDrawModeStandin expects to find one on the model it replaces.
      Without it the cards, and the bounds, collapse to the origin and draw nothing. The bound is
      computed when it is first asked for, once for the prim, at the earliest time as
      UsdImagingDrawModeAdapter did for a prim whose extent does not vary, and is left out when it
      is empty. It is not computed again when the children change.

    This scene index goes ahead of UsdImagingExtentResolvingSceneIndex, which still resolves an
    extentsHint where there is one, and ahead of instancing and of the draw mode scene index, which
    read the extent of the prim at a stage path. UsdImagingStageSceneIndex already dirties the
    extent when an authored extent or extentsHint changes, so this scene index sends no notices of
    its own.

    \class  HdVP2FallbackExtentSceneIndex
*/
class HdVP2FallbackExtentSceneIndex final : public HdSingleInputFilteringSceneIndexBase
{
public:
    static HdVP2FallbackExtentSceneIndexRefPtr New(const HdSceneIndexBaseRefPtr& inputSceneIndex);

    //! \brief  Sets the stage the extents are read from. Call before the input is populated.
    void SetStage(const UsdStageRefPtr& stage);

    HdSceneIndexPrim GetPrim(const SdfPath& primPath) const override;
    SdfPathVector    GetChildPrimPaths(const SdfPath& primPath) const override;

protected:
    HdVP2FallbackExtentSceneIndex(const HdSceneIndexBaseRefPtr& inputSceneIndex);

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
    UsdStageWeakPtr _stage;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
