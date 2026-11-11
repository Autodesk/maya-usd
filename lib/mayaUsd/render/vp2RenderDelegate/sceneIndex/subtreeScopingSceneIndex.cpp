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
#include <pxr/imaging/hd/dataSource.h>
#include <pxr/imaging/hd/instancerTopologySchema.h>
#include <pxr/imaging/hd/materialBindingsSchema.h>
#include <pxr/imaging/hd/overlayContainerDataSource.h>
#include <pxr/imaging/hd/retainedDataSource.h>
#include <pxr/imaging/hd/sceneIndexPrimView.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hd/xformSchema.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usdImaging/usdImaging/geomModelSchema.h>
#include <pxr/usdImaging/usdImaging/usdPrimInfoSchema.h>

#include <algorithm>
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

//! Data source layered over the root prim's own, replacing only its xform.
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

/*! \brief  The input container with one entry replaced, or hidden if the
            replacement is null.
*/
class _FilteredContainer : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_FilteredContainer);

    _FilteredContainer(
        const HdContainerDataSourceHandle& input,
        const TfToken&                     name,
        const HdDataSourceBaseHandle&      replacement)
        : _input(input)
        , _name(name)
        , _replacement(replacement)
    {
    }

    TfTokenVector GetNames() override
    {
        TfTokenVector names = _input->GetNames();
        if (!_replacement) {
            names.erase(std::remove(names.begin(), names.end(), _name), names.end());
        }
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        return name == _name ? _replacement : _input->Get(name);
    }

private:
    const HdContainerDataSourceHandle _input;
    const TfToken                     _name;
    const HdDataSourceBaseHandle      _replacement;
};

/*! \brief  What a prim outside the scope keeps: all its data, but not its type,
            and not what makes instancing aggregate it.

    Prims in scope read other prims' data by path:
    UsdImagingMaterialBindingsResolvingSceneIndex reads a collection binding's
    membership from the prim that owns the collection, UsdSkelImaging reads a
    skeleton's animation source and blend shape targets, and so can any scene
    index or schema added later. UsdImagingDelegate resolved all of them on the
    stage, wherever they were authored, so the data is kept whole instead of
    listing what is read.

    Without a type, the prim creates no rprim or sprim, and the filters that key
    off the type ignore it. UsdImagingNiInstanceAggregationSceneIndex keys off
    the usdPrimInfo niPrototypePath instead, and would aggregate an out-of-scope
    native instance into an instancer that draws it.
*/
HdContainerDataSourceHandle _OutsideDataSource(const HdContainerDataSourceHandle& input)
{
    if (!input) {
        return nullptr;
    }

    HdContainerDataSourceHandle primInfo
        = UsdImagingUsdPrimInfoSchema::GetFromParent(input).GetContainer();
    if (!primInfo) {
        return input;
    }

    // Not an overlay: it would merge the replacement with the input's own
    // usdPrimInfo again, and niPrototypePath would be back.
    return _FilteredContainer::New(
        input,
        UsdImagingUsdPrimInfoSchema::GetSchemaToken(),
        _FilteredContainer::New(
            primInfo, UsdImagingUsdPrimInfoSchemaTokens->niPrototypePath, nullptr));
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
    const HdSceneIndexBase& /* sender */,
    const HdSceneIndexObserver::AddedPrimEntries& entries)
{
    if (_rootPath == SdfPath::AbsoluteRootPath()) {
        _SendPrimsAdded(entries);
        return;
    }

    // The prototypes first, so the types below already account for them.
    for (const HdSceneIndexObserver::AddedPrimEntry& entry : entries) {
        if (entry.primType == HdPrimTypeTokens->instancer && _IsInScopeByPath(entry.primPath)) {
            _UpdateInstancer(entry.primPath);
        }
    }
    const SdfPathVector changedRoots = _RefreshPrototypeRoots();

    // The same types GetPrim reports.
    HdSceneIndexObserver::AddedPrimEntries filtered(entries);
    for (HdSceneIndexObserver::AddedPrimEntry& entry : filtered) {
        entry.primType = _ReportedType(entry.primPath, entry.primType);
    }
    _SendPrimsAdded(filtered);

    _ResyncPrototypes(changedRoots);
}

void HdVP2SubtreeScopingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase& /* sender */,
    const HdSceneIndexObserver::RemovedPrimEntries& entries)
{
    _SendPrimsRemoved(entries);

    if (_instancerPrototypes.empty()) {
        return;
    }
    for (const HdSceneIndexObserver::RemovedPrimEntry& entry : entries) {
        for (auto it = _instancerPrototypes.begin(); it != _instancerPrototypes.end();) {
            it = it->first.HasPrefix(entry.primPath) ? _instancerPrototypes.erase(it)
                                                     : std::next(it);
        }
    }
    _ResyncPrototypes(_RefreshPrototypeRoots());
}

void HdVP2SubtreeScopingSceneIndex::DirtyMaterialBindings(const SdfPathVector& primPaths)
{
    HdSceneIndexObserver::DirtiedPrimEntries entries;
    entries.reserve(primPaths.size());
    for (const SdfPath& primPath : primPaths) {
        entries.emplace_back(primPath, HdMaterialBindingsSchema::GetDefaultLocator());
    }
    _SendPrimsDirtied(entries);
}

void HdVP2SubtreeScopingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase& /* sender */,
    const HdSceneIndexObserver::DirtiedPrimEntries& entries)
{
    // Passed through, as HdsiPrimTypeAndPathPruningSceneIndex does. Prims in
    // scope depend on some of them: the data an outside prim keeps is read by
    // path, and material binding resolution and UsdSkelImaging re-resolve when
    // it is dirtied.
    _SendPrimsDirtied(entries);

    bool prototypesDirtied = false;
    for (const HdSceneIndexObserver::DirtiedPrimEntry& entry : entries) {
        if (_instancerPrototypes.count(entry.primPath)
            && entry.dirtyLocators.Intersects(HdInstancerTopologySchema::GetDefaultLocator())) {
            _UpdateInstancer(entry.primPath);
            prototypesDirtied = true;
        }
    }
    if (prototypesDirtied) {
        _ResyncPrototypes(_RefreshPrototypeRoots());
    }
}

TfToken
HdVP2SubtreeScopingSceneIndex::_ReportedType(const SdfPath& primPath, const TfToken& primType) const
{
    const _Scope scope = _Classify(primPath, primType);
    return scope == _Scope::Ancestor || scope == _Scope::Outside ? TfToken() : primType;
}

bool HdVP2SubtreeScopingSceneIndex::_IsInScopeByPath(const SdfPath& primPath) const
{
    return primPath.HasPrefix(_rootPath);
}

bool HdVP2SubtreeScopingSceneIndex::_IsInPrototype(const SdfPath& primPath) const
{
    // The roots are not nested, and a path sorts right before its descendants,
    // so the closest root at or before this path is the only one that can be
    // its ancestor.
    auto root = _prototypeRoots.upper_bound(primPath);
    return root != _prototypeRoots.begin() && primPath.HasPrefix(*--root);
}

void HdVP2SubtreeScopingSceneIndex::_UpdateInstancer(const SdfPath& instancerPath)
{
    const HdPathArrayDataSourceHandle prototypes
        = HdInstancerTopologySchema::GetFromParent(
              _GetInputSceneIndex()->GetPrim(instancerPath).dataSource)
              .GetPrototypes();
    if (!prototypes) {
        _instancerPrototypes.erase(instancerPath);
        return;
    }
    const VtArray<SdfPath> paths = prototypes->GetTypedValue(0.0f);
    _instancerPrototypes[instancerPath] = SdfPathVector(paths.begin(), paths.end());
}

SdfPathVector HdVP2SubtreeScopingSceneIndex::_RefreshPrototypeRoots()
{
    SdfPathVector all;
    for (const auto& instancerPrototypes : _instancerPrototypes) {
        all.insert(all.end(), instancerPrototypes.second.begin(), instancerPrototypes.second.end());
    }
    SdfPath::RemoveDescendentPaths(&all);
    std::set<SdfPath> roots(all.begin(), all.end());

    SdfPathVector changed;
    std::set_symmetric_difference(
        roots.begin(),
        roots.end(),
        _prototypeRoots.begin(),
        _prototypeRoots.end(),
        std::back_inserter(changed));
    _prototypeRoots = std::move(roots);
    return changed;
}

void HdVP2SubtreeScopingSceneIndex::_ResyncPrototypes(const SdfPathVector& roots)
{
    HdSceneIndexObserver::AddedPrimEntries entries;
    for (const SdfPath& root : roots) {
        for (const SdfPath& path : HdSceneIndexPrimView(_GetInputSceneIndex(), root)) {
            const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(path);
            // A prim that is gone, or not there yet, is reported by its own notice.
            if (prim.dataSource || !prim.primType.IsEmpty()) {
                entries.push_back({ path, _ReportedType(path, prim.primType) });
            }
        }
    }
    if (!entries.empty()) {
        _SendPrimsAdded(entries);
    }
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
    // are never aggregated, so they are not drawn. Point instancer prototypes
    // outside the root are kept the same way, but only those an instancer in
    // scope names.
    if (primType == HdPrimTypeTokens->material || UsdPrim::IsPathInPrototype(primPath.GetPrimPath())
        || _IsInPrototype(primPath)) {
        return _Scope::PassThrough;
    }
    return _Scope::Outside;
}

PXR_NAMESPACE_CLOSE_SCOPE
