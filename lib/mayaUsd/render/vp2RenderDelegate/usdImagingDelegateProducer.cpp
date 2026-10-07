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
#include "usdImagingDelegateProducer.h"

#include <pxr/base/tf/diagnostic.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usdImaging/usdImaging/version.h>

PXR_NAMESPACE_OPEN_SCOPE

void HdVP2UsdImagingDelegateProducer::Initialize(
    HdRenderIndex* renderIndex,
    const SdfPath& producerId)
{
    _renderIndex = renderIndex;
    _delegate.reset(new UsdImagingDelegate(renderIndex, producerId));
}

bool HdVP2UsdImagingDelegateProducer::Populate(
    const UsdStageRefPtr& stage,
    const SdfPath&        rootPath,
    const SdfPathVector&  excludedPaths)
{
    if (!TF_VERIFY(_delegate) || !TF_VERIFY(stage)) {
        return false;
    }

    // Remove any excluded prims before populating.
    for (const auto& excludePrim : excludedPaths) {
        const SdfPath indexPath = ToIndexPath(excludePrim);
        if (_renderIndex->HasRprim(indexPath)) {
            _renderIndex->RemoveRprim(indexPath);
        }
    }

    // The same prim ProxyShape()->usdPrim() resolves. An invalid prim is passed
    // through on purpose: UsdImagingDelegate reports it and populates nothing,
    // and Populate can only be called once per delegate, so returning false
    // here would retry, and report, on every frame.
    _delegate->Populate(stage->GetPrimAtPath(rootPath), excludedPaths);
    return true;
}

void HdVP2UsdImagingDelegateProducer::ApplyPendingUpdates()
{
    // The scene delegate applies stage edits through its own notice handling.
}

bool HdVP2UsdImagingDelegateProducer::FlushDeferredUpdates()
{
    // Marks go straight to HdChangeTracker, which syncs them within the same
    // SyncAll when raised during sprim sync, so nothing is ever withheld.
    return false;
}

void HdVP2UsdImagingDelegateProducer::SetTime(const UsdTimeCode& timeCode)
{
    _delegate->SetTime(timeCode);
}

UsdTimeCode HdVP2UsdImagingDelegateProducer::GetTime() const { return _delegate->GetTime(); }

void HdVP2UsdImagingDelegateProducer::SetRootTransform(const GfMatrix4d& transform)
{
    _delegate->SetRootTransform(transform);
}

GfMatrix4d HdVP2UsdImagingDelegateProducer::GetRootTransform() const
{
    return _delegate->GetRootTransform();
}

void HdVP2UsdImagingDelegateProducer::SetRootVisibility(bool visible)
{
    _delegate->SetRootVisibility(visible);
}

bool HdVP2UsdImagingDelegateProducer::GetRootVisibility() const
{
    return _delegate->GetRootVisibility();
}

void HdVP2UsdImagingDelegateProducer::SetRefineLevelFallback(int level)
{
    _delegate->SetRefineLevelFallback(level);
}

int HdVP2UsdImagingDelegateProducer::GetRefineLevelFallback() const
{
    return _delegate->GetRefineLevelFallback();
}

SdfPath HdVP2UsdImagingDelegateProducer::ToIndexPath(const SdfPath& usdPath) const
{
    return _delegate->ConvertCachePathToIndexPath(usdPath);
}

// Resolves an rprimId and instanceIndex back to the original USD gprim and instance index.
// see UsdImagingDelegate::GetScenePrimPath.
// This version works against all the older versions of USD we care about. Once those old
// versions go away, and we only support USD_IMAGING_API_VERSION >= 14 then we can remove
// this function.
SdfPath HdVP2UsdImagingDelegateProducer::GetScenePrimPath(
    const SdfPath&      rprimId,
    int                 instanceIndex,
    HdInstancerContext* instancerContext) const
{
#if defined(USD_IMAGING_API_VERSION) && USD_IMAGING_API_VERSION >= 16
    // Can no longer pass ALL_INSTANCES as the instanceIndex
    SdfPath usdPath = (instanceIndex == UsdImagingDelegate::ALL_INSTANCES)
        ? rprimId.ReplacePrefix(_delegate->GetDelegateID(), SdfPath::AbsoluteRootPath())
        : _delegate->GetScenePrimPath(rprimId, instanceIndex, instancerContext);
#elif defined(USD_IMAGING_API_VERSION) && USD_IMAGING_API_VERSION >= 14
    SdfPath usdPath = _delegate->GetScenePrimPath(rprimId, instanceIndex, instancerContext);
#elif defined(USD_IMAGING_API_VERSION) && USD_IMAGING_API_VERSION >= 13
    SdfPath usdPath = _delegate->GetScenePrimPath(rprimId, instanceIndex);
#else
    SdfPath indexPath;
    if (drawInstID > 0) {
        indexPath = _delegate->GetPathForInstanceIndex(rprimId, instanceIndex, nullptr);
    } else {
        indexPath = rprimId;
    }

    SdfPath usdPath = _delegate->ConvertIndexPathToCachePath(indexPath);

    // Examine the USD path. If it is not a valid prim path, the selection hit is from a single
    // instance Rprim and indexPath is actually its instancer Rprim id. In this case we should
    // call GetPathForInstanceIndex() using 0 as the instance index.
    if (!usdPath.IsPrimPath()) {
        indexPath = _delegate->GetPathForInstanceIndex(rprimId, 0, nullptr);
        usdPath = _delegate->ConvertIndexPathToCachePath(indexPath);
    }

    // The "Instances" point instances pick mode is not supported for
    // USD_IMAGING_API_VERSION < 14 (core USD versions earlier than 20.08), so
    // no using instancerContext here.
#endif

    return usdPath;
}

SdfPathVector HdVP2UsdImagingDelegateProducer::GetScenePrimPaths(
    const SdfPath&          rprimId,
    const std::vector<int>& instanceIndexes) const
{
#if defined(USD_IMAGING_API_VERSION) && USD_IMAGING_API_VERSION >= 17
    return _delegate->GetScenePrimPaths(rprimId, instanceIndexes);
#else
    SdfPathVector usdPaths;
    usdPaths.reserve(instanceIndexes.size());
    for (int instanceIndex : instanceIndexes) {
        usdPaths.emplace_back(GetScenePrimPath(rprimId, instanceIndex, nullptr));
    }
    return usdPaths;
#endif
}

void HdVP2UsdImagingDelegateProducer::PopulateSelection(
    const SdfPath&              usdPath,
    int                         instanceIndex,
    const HdSelectionSharedPtr& result)
{
    SdfPath path = usdPath;

#if !defined(USD_IMAGING_API_VERSION) || USD_IMAGING_API_VERSION < 11
    path = _delegate->ConvertCachePathToIndexPath(path);
#endif

    _delegate->PopulateSelection(HdSelection::HighlightModeSelect, path, instanceIndex, result);
}

// UsdImagingDelegate is a legacy scene delegate, so its prims are in the render
// index's HdLegacyPrimSceneIndex whether or not emulation is on. The change
// tracker reaches them either way.
void HdVP2UsdImagingDelegateProducer::MarkRprimDirty(const SdfPath& indexPath, HdDirtyBits bits)
{
    if (_renderIndex && _renderIndex->HasRprim(indexPath)) {
        _renderIndex->GetChangeTracker().MarkRprimDirty(indexPath, bits);
    }
}

void HdVP2UsdImagingDelegateProducer::MarkSprimDirty(const SdfPath& indexPath, HdDirtyBits bits)
{
    if (_renderIndex) {
        _renderIndex->GetChangeTracker().MarkSprimDirty(indexPath, bits);
    }
}

HdSceneDelegate* HdVP2UsdImagingDelegateProducer::GetSceneDelegate() const
{
    return _delegate.get();
}

UsdImagingDelegate* HdVP2UsdImagingDelegateProducer::GetUsdImagingDelegate() const
{
    return _delegate.get();
}

PXR_NAMESPACE_CLOSE_SCOPE
