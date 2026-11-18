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
#include "implicitDoubleSidedSceneIndex.h"

#include <pxr/base/tf/staticTokens.h>
#include <pxr/imaging/hd/dataSource.h>
#include <pxr/imaging/hd/dataSourceLocator.h>
#include <pxr/imaging/hd/meshSchema.h>
#include <pxr/imaging/hd/overlayContainerDataSource.h>
#include <pxr/imaging/hd/retainedDataSource.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/tokens.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// The container the tag lives in. It is not a schema: nothing outside this file reads it.
TF_DEFINE_PRIVATE_TOKENS(_tokens, (vp2ImplicitSurface));

using Time = HdSampledDataSource::Time;

//! Where the restoring scene index finds the tag. The last element is the name the mesh schema
//! gives the value.
const HdDataSourceLocator& _TagLocator()
{
    static const HdDataSourceLocator locator(
        _tokens->vp2ImplicitSurface, HdMeshSchemaTokens->doubleSided);
    return locator;
}

/*! The prim types HdsiImplicitSurfaceSceneIndex hard-codes doubleSided = false for. The plane
    is not one of them: HdPlaneSchema carries doubleSided, and the conversion copies it.
*/
bool _IsImplicitSurface(const TfToken& primType)
{
    return primType == HdPrimTypeTokens->cube || primType == HdPrimTypeTokens->sphere
        || primType == HdPrimTypeTokens->cone || primType == HdPrimTypeTokens->cylinder
        || primType == HdPrimTypeTokens->capsule;
}

/*! Reads the attribute from the stage on every query, so it follows edits as soon as the prim is
    dirtied. doubleSided is uniform, so it has no time samples.
*/
class _DoubleSidedDataSource : public HdBoolDataSource
{
public:
    HD_DECLARE_DATASOURCE(_DoubleSidedDataSource);

    VtValue GetValue(const Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    bool GetTypedValue(const Time /* shutterOffset */) override
    {
        bool doubleSided = false;
        if (_stage) {
            const UsdPrim prim = _stage->GetPrimAtPath(_primPath);
            if (prim) {
                // Unauthored, the attribute reads as its schema fallback, false. It is missing
                // only if the prim stopped being a gprim, and false is right then too.
                prim.GetAttribute(UsdGeomTokens->doubleSided)
                    .Get(&doubleSided, UsdTimeCode::Default());
            }
        }
        return doubleSided;
    }

    bool GetContributingSampleTimesForInterval(
        const Time /* startTime */,
        const Time /* endTime */,
        std::vector<Time>* const /* outSampleTimes */) override
    {
        return false;
    }

private:
    _DoubleSidedDataSource(const UsdStageWeakPtr& stage, const SdfPath& primPath)
        : _stage(stage)
        , _primPath(primPath)
    {
    }

    const UsdStageWeakPtr _stage;
    const SdfPath         _primPath;
};

HdContainerDataSourceHandle _Tag(const UsdStageWeakPtr& stage, const SdfPath& primPath)
{
    return HdRetainedContainerDataSource::New(
        _tokens->vp2ImplicitSurface,
        HdRetainedContainerDataSource::New(
            HdMeshSchemaTokens->doubleSided, _DoubleSidedDataSource::New(stage, primPath)));
}

//! The mesh's doubleSided, which the conversion set to false, replaced by the tag.
HdContainerDataSourceHandle _MeshOverlay(const HdDataSourceBaseHandle& tag)
{
    return HdRetainedContainerDataSource::New(
        HdMeshSchema::GetSchemaToken(),
        HdRetainedContainerDataSource::New(HdMeshSchemaTokens->doubleSided, tag));
}

} // namespace

// -----------------------------------------------------------------------------
// HdVP2DoubleSidedTaggingSceneIndex
// -----------------------------------------------------------------------------

HdVP2DoubleSidedTaggingSceneIndexRefPtr
HdVP2DoubleSidedTaggingSceneIndex::New(const HdSceneIndexBaseRefPtr& inputSceneIndex)
{
    return TfCreateRefPtr(new HdVP2DoubleSidedTaggingSceneIndex(inputSceneIndex));
}

HdVP2DoubleSidedTaggingSceneIndex::HdVP2DoubleSidedTaggingSceneIndex(
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
    : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
}

HdVP2DoubleSidedTaggingSceneIndex::~HdVP2DoubleSidedTaggingSceneIndex()
{
    TfNotice::Revoke(_noticeKey);
}

void HdVP2DoubleSidedTaggingSceneIndex::SetStage(const UsdStageRefPtr& stage)
{
    TfNotice::Revoke(_noticeKey);

    _stage = stage;
    if (_stage) {
        _noticeKey = TfNotice::Register(
            TfCreateWeakPtr(this), &HdVP2DoubleSidedTaggingSceneIndex::_OnObjectsChanged, _stage);
    }
}

HdSceneIndexPrim HdVP2DoubleSidedTaggingSceneIndex::GetPrim(const SdfPath& primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (_stage && prim.dataSource && _IsImplicitSurface(prim.primType)) {
        prim.dataSource
            = HdOverlayContainerDataSource::New(_Tag(_stage, primPath), prim.dataSource);
    }
    return prim;
}

SdfPathVector HdVP2DoubleSidedTaggingSceneIndex::GetChildPrimPaths(const SdfPath& primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void HdVP2DoubleSidedTaggingSceneIndex::ApplyPendingUpdates()
{
    SdfPathSet paths;
    {
        std::lock_guard<std::mutex> lock(_pendingMutex);
        paths.swap(_pendingPaths);
    }
    if (paths.empty()) {
        return;
    }

    // The locator the restoring scene index publishes the value at. Nothing between the two
    // scene indices rewrites it: the conversion forwards dirty locators as they are.
    static const HdDataSourceLocatorSet locators(HdMeshSchema::GetDoubleSidedLocator());

    HdSceneIndexObserver::DirtiedPrimEntries entries;
    for (const SdfPath& path : paths) {
        // A prim the stage scene index has just removed or turned into another type is
        // reported by its own notice, and needs nothing from here.
        if (_IsImplicitSurface(_GetInputSceneIndex()->GetPrim(path).primType)) {
            entries.emplace_back(path, locators);
        }
    }
    if (!entries.empty()) {
        _SendPrimsDirtied(entries);
    }
}

void HdVP2DoubleSidedTaggingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase& /* sender */,
    const HdSceneIndexObserver::AddedPrimEntries& entries)
{
    _SendPrimsAdded(entries);
}

void HdVP2DoubleSidedTaggingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase& /* sender */,
    const HdSceneIndexObserver::RemovedPrimEntries& entries)
{
    _SendPrimsRemoved(entries);
}

void HdVP2DoubleSidedTaggingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase& /* sender */,
    const HdSceneIndexObserver::DirtiedPrimEntries& entries)
{
    _SendPrimsDirtied(entries);
}

void HdVP2DoubleSidedTaggingSceneIndex::_OnObjectsChanged(
    const UsdNotice::ObjectsChanged& notice,
    const UsdStageWeakPtr&           sender)
{
    if (sender != _stage) {
        return;
    }

    // A property that was just authored or removed arrives as a resync, one that changed value
    // as an info-only change; UsdImagingStageSceneIndex reads both the same way. A resync of the
    // prim itself repopulates it, so the tag is rebuilt without help.
    SdfPathVector edited;
    const auto    collect = [&edited](const UsdNotice::ObjectsChanged::PathRange& paths) {
        for (const SdfPath& path : paths) {
            if (path.IsPropertyPath() && path.GetNameToken() == UsdGeomTokens->doubleSided) {
                edited.push_back(path.GetPrimPath());
            }
        }
    };
    collect(notice.GetResyncedPaths());
    collect(notice.GetChangedInfoOnlyPaths());
    if (edited.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(_pendingMutex);
    _pendingPaths.insert(edited.begin(), edited.end());
}

// -----------------------------------------------------------------------------
// HdVP2DoubleSidedRestoringSceneIndex
// -----------------------------------------------------------------------------

HdVP2DoubleSidedRestoringSceneIndexRefPtr
HdVP2DoubleSidedRestoringSceneIndex::New(const HdSceneIndexBaseRefPtr& inputSceneIndex)
{
    return TfCreateRefPtr(new HdVP2DoubleSidedRestoringSceneIndex(inputSceneIndex));
}

HdVP2DoubleSidedRestoringSceneIndex::HdVP2DoubleSidedRestoringSceneIndex(
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
    : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
}

HdSceneIndexPrim HdVP2DoubleSidedRestoringSceneIndex::GetPrim(const SdfPath& primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (prim.primType == HdPrimTypeTokens->mesh && prim.dataSource) {
        if (const HdDataSourceBaseHandle tag
            = HdContainerDataSource::Get(prim.dataSource, _TagLocator())) {
            prim.dataSource = HdOverlayContainerDataSource::New(_MeshOverlay(tag), prim.dataSource);
        }
    }
    return prim;
}

SdfPathVector HdVP2DoubleSidedRestoringSceneIndex::GetChildPrimPaths(const SdfPath& primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void HdVP2DoubleSidedRestoringSceneIndex::_PrimsAdded(
    const HdSceneIndexBase& /* sender */,
    const HdSceneIndexObserver::AddedPrimEntries& entries)
{
    _SendPrimsAdded(entries);
}

void HdVP2DoubleSidedRestoringSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase& /* sender */,
    const HdSceneIndexObserver::RemovedPrimEntries& entries)
{
    _SendPrimsRemoved(entries);
}

void HdVP2DoubleSidedRestoringSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase& /* sender */,
    const HdSceneIndexObserver::DirtiedPrimEntries& entries)
{
    _SendPrimsDirtied(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
