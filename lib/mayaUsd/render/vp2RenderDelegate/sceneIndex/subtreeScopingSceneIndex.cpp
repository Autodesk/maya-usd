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
#include "subtreeScopingSceneIndex.h"

#include <pxr/base/gf/matrix4d.h>
#include <pxr/imaging/hd/collectionsSchema.h>
#include <pxr/imaging/hd/overlayContainerDataSource.h>
#include <pxr/imaging/hd/retainedDataSource.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hd/xformSchema.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usdImaging/usdImaging/geomModelSchema.h>

#include <iterator>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

HdContainerDataSourceHandle _IdentityXform()
{
    static const HdContainerDataSourceHandle xform
        = HdXformSchema::Builder()
              .SetMatrix(HdRetainedTypedSampledDataSource<GfMatrix4d>::New(GfMatrix4d(1.0)))
              .SetResetXformStack(HdRetainedTypedSampledDataSource<bool>::New(false))
              .Build();
    return xform;
}

//! Overlaid on the root prim: identity in place of its own transform.
HdContainerDataSourceHandle _RootPrimOverlay()
{
    static const HdContainerDataSourceHandle overlay
        = HdRetainedContainerDataSource::New(HdXformSchema::GetSchemaToken(), _IdentityXform());
    return overlay;
}

/*! \brief  Overlaid on the root prim's ancestors.

    Identity, so their transforms are not applied twice, and no applyDrawMode,
    because UsdImagingDelegate never traversed an ancestor, so an ancestor never
    replaced the subtree with its draw mode. drawMode itself is kept, since it
    still inherits to components in scope.
*/
HdContainerDataSourceHandle _AncestorOverlay()
{
    static const HdContainerDataSourceHandle overlay = HdRetainedContainerDataSource::New(
        HdXformSchema::GetSchemaToken(),
        _IdentityXform(),
        UsdImagingGeomModelSchema::GetSchemaToken(),
        UsdImagingGeomModelSchema::Builder()
            .SetApplyDrawMode(HdRetainedTypedSampledDataSource<bool>::New(false))
            .Build());
    return overlay;
}

/*! \brief  What a prim outside the scope keeps: the data that prims in scope
            read from it by path.

    UsdImagingMaterialBindingsResolvingSceneIndex reads a collection binding's
    membership from the prim that owns the collection, and UsdSkelImaging reads
    a skeleton's skel:animationSource and a skinned prim's blend shape targets
    from the prims they name. UsdImagingDelegate resolved all of them on the
    stage, wherever they were authored. The prim stays typeless, so nothing new
    is drawn.
*/
HdContainerDataSourceHandle _OutsideDataSource(const HdContainerDataSourceHandle& input)
{
    // The skel tokens are UsdSkelImaging's animation and blend shape schema
    // tokens; usdSkelImaging is not linked.
    static const TfToken names[] = { HdCollectionsSchema::GetSchemaToken(),
                                     TfToken("skelAnimation"),
                                     TfToken("skelBlendShape") };

    if (!input) {
        return nullptr;
    }
    TfToken                keptNames[std::size(names)];
    HdDataSourceBaseHandle kept[std::size(names)];
    size_t                 count = 0;
    for (const TfToken& name : names) {
        if (HdDataSourceBaseHandle dataSource = input->Get(name)) {
            keptNames[count] = name;
            kept[count++] = dataSource;
        }
    }
    return count ? HdRetainedContainerDataSource::New(count, keptNames, kept) : nullptr;
}

} // namespace

HdVP2SubtreeScopingSceneIndexRefPtr
HdVP2SubtreeScopingSceneIndex::New(const HdSceneIndexBaseRefPtr& inputSceneIndex)
{
    return TfCreateRefPtr(new HdVP2SubtreeScopingSceneIndex(inputSceneIndex));
}

HdVP2SubtreeScopingSceneIndex::HdVP2SubtreeScopingSceneIndex(
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
    : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
}

HdSceneIndexPrim HdVP2SubtreeScopingSceneIndex::GetPrim(const SdfPath& primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    switch (_Classify(primPath, prim.primType)) {
    case _Scope::PassThrough: return prim;
    case _Scope::RootPrim:
        if (prim.dataSource) {
            prim.dataSource
                = HdOverlayContainerDataSource::New(_RootPrimOverlay(), prim.dataSource);
        }
        return prim;
    case _Scope::Ancestor:
        return { TfToken(),
                 prim.dataSource
                     ? HdOverlayContainerDataSource::New(_AncestorOverlay(), prim.dataSource)
                     : nullptr };
    case _Scope::Outside: return { TfToken(), _OutsideDataSource(prim.dataSource) };
    }

    return prim;
}

SdfPathVector HdVP2SubtreeScopingSceneIndex::GetChildPrimPaths(const SdfPath& primPath) const
{
    // Unfiltered, so traversal still reaches materials and prototypes outside
    // the subtree.
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void HdVP2SubtreeScopingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase&                       /* sender */,
    const HdSceneIndexObserver::AddedPrimEntries& entries)
{
    if (_rootPath == SdfPath::AbsoluteRootPath()) {
        _SendPrimsAdded(entries);
        return;
    }

    // The same types GetPrim reports.
    HdSceneIndexObserver::AddedPrimEntries filtered(entries);
    for (HdSceneIndexObserver::AddedPrimEntry& entry : filtered) {
        const _Scope scope = _Classify(entry.primPath, entry.primType);
        if (scope == _Scope::Ancestor || scope == _Scope::Outside) {
            entry.primType = TfToken();
        }
    }
    _SendPrimsAdded(filtered);
}

void HdVP2SubtreeScopingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase&                         /* sender */,
    const HdSceneIndexObserver::RemovedPrimEntries& entries)
{
    _SendPrimsRemoved(entries);
}

void HdVP2SubtreeScopingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase&                         /* sender */,
    const HdSceneIndexObserver::DirtiedPrimEntries& entries)
{
    // Passed through, as HdsiPrimTypeAndPathPruningSceneIndex does. Prims in
    // scope depend on some of them: the data an outside prim keeps is read by
    // path, and material binding resolution and UsdSkelImaging re-resolve when
    // it is dirtied.
    _SendPrimsDirtied(entries);
}

HdVP2SubtreeScopingSceneIndex::_Scope
HdVP2SubtreeScopingSceneIndex::_Classify(const SdfPath& primPath, const TfToken& primType) const
{
    if (_rootPath == SdfPath::AbsoluteRootPath() || primPath.IsAbsoluteRootPath()) {
        return _Scope::PassThrough;
    }
    if (primPath == _rootPath) {
        return _Scope::RootPrim;
    }
    if (primPath.HasPrefix(_rootPath)) {
        return _Scope::PassThrough;
    }
    if (_rootPath.HasPrefix(primPath)) {
        return _Scope::Ancestor;
    }
    // Materials are kept, for the unbound material pruning further down to
    // remove unless something in scope binds them. Prototypes are kept whole:
    // instances in scope draw from them, and those nothing in scope instances
    // are never aggregated, so they are not drawn.
    if (primType == HdPrimTypeTokens->material
        || UsdPrim::IsPathInPrototype(primPath.GetPrimPath())) {
        return _Scope::PassThrough;
    }
    return _Scope::Outside;
}

PXR_NAMESPACE_CLOSE_SCOPE
