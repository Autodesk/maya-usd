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
#include "legacyMaterialBindingsSceneIndex.h"

#include <pxr/base/tf/stringUtils.h>
#include <pxr/imaging/hd/dataSource.h>
#include <pxr/imaging/hd/dataSourceLocator.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/tokens.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/tokens.h>
#include <pxr/usdImaging/usdImaging/dataSourceStageGlobals.h>
#include <pxr/usdImaging/usdImaging/materialBindingAPIAdapter.h>
#include <pxr/usdImaging/usdImaging/materialBindingsSchema.h>

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

/*! The adapter UsdImagingStageSceneIndex uses for a prim that applies MaterialBindingAPI. It reads
    the bindings through UsdShadeMaterialBindingAPI, which does not need the schema to be applied,
    so asking it about any prim gives the data the prim would have if it did. It keeps no state.
*/
UsdImagingMaterialBindingAPIAdapter& _Adapter()
{
    static UsdImagingMaterialBindingAPIAdapter adapter;
    return adapter;
}

//! What the adapter is given in place of the stage's. It ignores it: relationships have no time.
class _NoStageGlobals final : public UsdImagingDataSourceStageGlobals
{
public:
    UsdTimeCode GetTime() const override { return UsdTimeCode::Default(); }

    void FlagAsTimeVarying(const SdfPath& /* hydraPath */, const HdDataSourceLocator& /* locator */)
        const override
    {
    }

    void FlagAsAssetPathDependent(const SdfPath& /* usdPath */) const override { }
};

/*! Whether a property is a binding, direct or by collection, for any purpose: its name starts with
    material:binding. This is what UsdShadeMaterialBindingAPI::BindingsAtPrim looks for when it
    honors legacy bindings.
*/
bool _IsBindingName(const TfToken& name)
{
    return TfStringStartsWith(name.GetString(), UsdShadeTokens->materialBinding.GetString());
}

/*! The properties UsdImagingMaterialBindingAPIAdapter reacts to when they change. Which of them
    dirty what is the adapter's to say; this only keeps edits of any other property from being
    collected at all.
*/
bool _IsBindingProperty(const TfToken& name)
{
    return _IsBindingName(name)
        || TfStringStartsWith(name.GetString(), UsdTokens->collection.GetString());
}

bool _AuthorsBindings(const UsdPrim& prim)
{
    return !prim.GetAuthoredPropertyNames(_IsBindingName).empty();
}

/*! The prim, and the bindings of the stage prim it stands for when it has some UsdImaging gave
    it none of. Asking the stage is left to the moment someone asks for them, so a prim nobody
    reads the bindings of costs the allocation of this object, and a prim that applies the schema
    keeps what UsdImaging gave it.
*/
class _PrimDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimDataSource);

    TfTokenVector GetNames() override
    {
        TfTokenVector names = _input->GetNames();
        if (std::find(names.begin(), names.end(), _BindingsToken()) == names.end()
            && _LegacyBindings()) {
            names.push_back(_BindingsToken());
        }
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        HdDataSourceBaseHandle result = _input->Get(name);
        if (!result && name == _BindingsToken()) {
            result = _LegacyBindings();
        }
        return result;
    }

private:
    _PrimDataSource(
        const HdContainerDataSourceHandle& input,
        const UsdStageWeakPtr&             stage,
        const SdfPath&                     primPath)
        : _input(input)
        , _stage(stage)
        , _primPath(primPath)
    {
    }

    static const TfToken& _BindingsToken()
    {
        return UsdImagingMaterialBindingsSchema::GetSchemaToken();
    }

    HdDataSourceBaseHandle _LegacyBindings() const
    {
        // The stage may be gone by the time a data source that outlived it is read.
        if (!_stage) {
            return nullptr;
        }

        const UsdPrim prim = _stage->GetPrimAtPath(_primPath);
        if (!prim || !_AuthorsBindings(prim)) {
            return nullptr;
        }

        const HdContainerDataSourceHandle data
            = _Adapter().GetImagingSubprimData(prim, TfToken(), TfToken(), _NoStageGlobals());
        return data ? data->Get(_BindingsToken()) : nullptr;
    }

    const HdContainerDataSourceHandle _input;
    const UsdStageWeakPtr             _stage;
    const SdfPath                     _primPath;
};

} // namespace

HdVP2LegacyMaterialBindingsSceneIndexRefPtr
HdVP2LegacyMaterialBindingsSceneIndex::New(const HdSceneIndexBaseRefPtr& inputSceneIndex)
{
    return TfCreateRefPtr(new HdVP2LegacyMaterialBindingsSceneIndex(inputSceneIndex));
}

HdVP2LegacyMaterialBindingsSceneIndex::HdVP2LegacyMaterialBindingsSceneIndex(
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
    : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
}

HdVP2LegacyMaterialBindingsSceneIndex::~HdVP2LegacyMaterialBindingsSceneIndex()
{
    TfNotice::Revoke(_noticeKey);
}

void HdVP2LegacyMaterialBindingsSceneIndex::SetStage(const UsdStageRefPtr& stage)
{
    TfNotice::Revoke(_noticeKey);

    _stage = stage;
    if (_stage) {
        _noticeKey = TfNotice::Register(
            TfCreateWeakPtr(this),
            &HdVP2LegacyMaterialBindingsSceneIndex::_OnObjectsChanged,
            _stage);
    }
}

HdSceneIndexPrim HdVP2LegacyMaterialBindingsSceneIndex::GetPrim(const SdfPath& primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    // Only a prim path names a stage prim: the others, a subprim of an adapter for instance, have
    // no bindings of their own.
    if (_stage && prim.dataSource && primPath.IsPrimPath()) {
        prim.dataSource = _PrimDataSource::New(prim.dataSource, _stage, primPath);
    }
    return prim;
}

SdfPathVector
HdVP2LegacyMaterialBindingsSceneIndex::GetChildPrimPaths(const SdfPath& primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void HdVP2LegacyMaterialBindingsSceneIndex::ApplyPendingUpdates()
{
    PropertyMap resynced;
    PropertyMap updated;
    {
        std::lock_guard<std::mutex> lock(_pendingMutex);
        resynced.swap(_pendingResynced);
        updated.swap(_pendingUpdated);
    }
    if (resynced.empty() && updated.empty()) {
        return;
    }

    // A prim given a binding is in both: the relationship was authored, then given its targets.
    // One entry for it, so that the bindings of its descendants are invalidated once.
    LocatorMap locators;
    _Collect(resynced, UsdImagingPropertyInvalidationType::Resync, &locators);
    _Collect(updated, UsdImagingPropertyInvalidationType::Update, &locators);

    HdSceneIndexObserver::DirtiedPrimEntries entries;
    entries.reserve(locators.size());
    for (const auto& [path, dirty] : locators) {
        entries.emplace_back(path, dirty);
    }
    if (!entries.empty()) {
        _SendPrimsDirtied(entries);
    }
}

void HdVP2LegacyMaterialBindingsSceneIndex::_Collect(
    const PropertyMap&                       properties,
    const UsdImagingPropertyInvalidationType type,
    LocatorMap* const                        locators) const
{
    for (const auto& [path, names] : properties) {
        const UsdPrim prim = _stage->GetPrimAtPath(path);

        // A prim that applies the schema was dirtied by UsdImagingStageSceneIndex, whose adapter
        // is registered for it. One that has just been removed, or is out of scope, is reported by
        // its own notice or not at all.
        if (!prim || prim.HasAPI<UsdShadeMaterialBindingAPI>()
            || !_GetInputSceneIndex()->GetPrim(path).dataSource) {
            continue;
        }

        const HdDataSourceLocatorSet dirty
            = _Adapter().InvalidateImagingSubprim(prim, TfToken(), TfToken(), names, type);
        if (!dirty.IsEmpty()) {
            (*locators)[path].insert(dirty);
        }
    }
}

void HdVP2LegacyMaterialBindingsSceneIndex::_PrimsAdded(
    const HdSceneIndexBase& /* sender */,
    const HdSceneIndexObserver::AddedPrimEntries& entries)
{
    _SendPrimsAdded(entries);
}

void HdVP2LegacyMaterialBindingsSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase& /* sender */,
    const HdSceneIndexObserver::RemovedPrimEntries& entries)
{
    _SendPrimsRemoved(entries);
}

void HdVP2LegacyMaterialBindingsSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase& /* sender */,
    const HdSceneIndexObserver::DirtiedPrimEntries& entries)
{
    _SendPrimsDirtied(entries);
}

void HdVP2LegacyMaterialBindingsSceneIndex::_OnObjectsChanged(
    const UsdNotice::ObjectsChanged& notice,
    const UsdStageWeakPtr&           sender)
{
    if (sender != _stage) {
        return;
    }

    // A property that was just authored or removed arrives as a resync, one that changed value as
    // an info-only change; UsdImagingStageSceneIndex tells them apart the same way. A resync of
    // the prim itself repopulates it, and the bindings are read again with it.
    PropertyMap resynced;
    PropertyMap updated;
    const auto  collect = [](const UsdNotice::ObjectsChanged::PathRange& paths, PropertyMap& into) {
        for (const SdfPath& path : paths) {
            if (path.IsPropertyPath() && _IsBindingProperty(path.GetNameToken())) {
                into[path.GetPrimPath()].push_back(path.GetNameToken());
            }
        }
    };
    collect(notice.GetResyncedPaths(), resynced);
    collect(notice.GetChangedInfoOnlyPaths(), updated);
    if (resynced.empty() && updated.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(_pendingMutex);
    for (auto& [path, names] : resynced) {
        TfTokenVector& pending = _pendingResynced[path];
        pending.insert(pending.end(), names.begin(), names.end());
    }
    for (auto& [path, names] : updated) {
        TfTokenVector& pending = _pendingUpdated[path];
        pending.insert(pending.end(), names.begin(), names.end());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
