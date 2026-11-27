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
#ifndef HD_VP2_POLYGONAL_NORMALS_SCENE_INDEX
#define HD_VP2_POLYGONAL_NORMALS_SCENE_INDEX

#include <pxr/imaging/hd/filteringSceneIndex.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdVP2PolygonalNormalsSceneIndex);

/*! \brief  Publishes the authored normals of a mesh only if it is polygonal, as
            UsdImagingMeshAdapter did.

    UsdGeomPointBased says that normals "should not be authored on any Mesh that is subdivided,
    since the subdivision algorithm will define its own normals". UsdImagingMeshAdapter followed
    it: for a mesh whose subdivisionScheme is not none it forwarded no normals, authored as
    primvars:normals, inherited, or as the normals attribute, and VP2 computed smooth normals for
    it. UsdImagingStageSceneIndex publishes them for every mesh, and VP2 computes smooth normals
    only for a mesh that has none, so a mesh that authors normals and no subdivisionScheme, which
    the schema's fallback, catmullClark, subdivides, is shaded by the normals it was to ignore.

    This scene index hides primvars/normals of a mesh whose subdivisionScheme is not none, so that
    VP2 is given what it was given. It also reports normals as dirty when the scheme is edited,
    because the edit reaches the render index as a topology change only, and the primvars would
    not be asked for again. A mesh that publishes no subdivisionScheme is polygonal to the
    emulation delegate (HdSceneIndexAdapterSceneDelegate::GetMeshTopology), so its normals stay.

    \class  HdVP2PolygonalNormalsSceneIndex
*/
class HdVP2PolygonalNormalsSceneIndex final : public HdSingleInputFilteringSceneIndexBase
{
public:
    static HdVP2PolygonalNormalsSceneIndexRefPtr New(const HdSceneIndexBaseRefPtr& inputSceneIndex);

    HdSceneIndexPrim GetPrim(const SdfPath& primPath) const override;
    SdfPathVector    GetChildPrimPaths(const SdfPath& primPath) const override;

protected:
    HdVP2PolygonalNormalsSceneIndex(const HdSceneIndexBaseRefPtr& inputSceneIndex);

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
