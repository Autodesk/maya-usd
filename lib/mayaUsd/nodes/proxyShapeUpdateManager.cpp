//
// Copyright 2023 Autodesk
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
#include "proxyShapeUpdateManager.h"

#include <pxr/base/vt/value.h>
#include <pxr/usd/sdf/listOp.h>
#include <pxr/usd/usd/tokens.h>
#include <pxr/usd/usdUI/tokens.h>

#include <mutex>

using namespace MAYAUSD_NS_DEF;

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// We have incoming changes that USD will consider either requiring an
// update (meaning the render delegate needs to refresh and redraw) or
// a resync (meaning the scene delegate needs to fetch new datum). We
// want external clients to be aware of these classes of updates in case
// they do not use the Hydra system for refreshing and drawing the scene.
enum class _UsdChangeType
{
    kIgnored, // Change does not require redraw: UI change, metadata change.
    kUpdate,  // Change requires redraw after refreshing parameter values
    kResync   // Change requires refreshing cached buffers
};

// If the notification is about prepending a UI schema, we don't want a refresh. These structures
// are quite large to inspect, but they hash easily, so let's compare known hashes.
bool _IsUiSchemaPrepend(const VtValue& v)
{
    static std::set<size_t> UiSchemaPrependHashes;
    std::once_flag          hasHashes;
    std::call_once(hasHashes, [&]() {
        SdfTokenListOp op;
        op.SetPrependedItems(TfTokenVector { TfToken("NodeGraphNodeAPI") });
        UiSchemaPrependHashes.insert(hash_value(op));
    });

    if (v.IsHolding<SdfTokenListOp>()) {
        const size_t hash = hash_value(v.UncheckedGet<SdfTokenListOp>());
        if (UiSchemaPrependHashes.count(hash)) {
            return true;
        }
    }
    return false;
}

// This is a stripped down copy of UsdImagingDelegate::_OnUsdObjectsChanged which is the main USD
// notification handler where paths to refresh and paths to update are compiled for the next Hydra
// refresh. We do not gather paths as there is no simple way to know when to flush these maps.
//
// This needs to stay as quick as possible since it is stuck in the middle of the notification code
// path.
//
// This is a work in progress. Some improvements might be necessary in the future. The following
// potential issues are already visible:
//
//  - Changing a parameter value for the first time creates the attribute, which is a kResync
_UsdChangeType _ClassifyUsdObjectsChanged(UsdNotice::ObjectsChanged const& notice)
{
    using PathRange = UsdNotice::ObjectsChanged::PathRange;

    auto range = notice.GetResyncedPaths();
    if (!range.empty()) {
        size_t ignoredCount = 0;
        size_t resyncCount = 0;
        for (auto it = range.begin(); it != range.end(); ++it) {
            if (it->IsPropertyPath()) {
                // We have a bunch of UI properties to ignore. Especially anything that comes from
                // UI schemas.
                if (it->GetName().rfind("ui:", 0) == 0) {
                    ++ignoredCount;
                    continue;
                }
            }
            for (const SdfChangeList::Entry* entry : it.base()->second) {
                for (auto&& infoChanged : entry->infoChanged) {
                    if (infoChanged.first == UsdTokens->apiSchemas
                        && _IsUiSchemaPrepend(infoChanged.second.second)) {
                        ++ignoredCount;
                    } else {
                        ++resyncCount;
                    }
                }
            }
        }

        if (ignoredCount) {
            return resyncCount ? _UsdChangeType::kResync : _UsdChangeType::kIgnored;
        } else {
            return _UsdChangeType::kResync;
        }
    }

    auto retVal = _UsdChangeType::kIgnored;

    const PathRange pathsToUpdate = notice.GetChangedInfoOnlyPaths();
    for (PathRange::const_iterator it = pathsToUpdate.begin(); it != pathsToUpdate.end(); ++it) {
        if (it->IsAbsoluteRootOrPrimPath()) {
            const TfTokenVector changedFields = it.GetChangedFields();
            if (!changedFields.empty()) {
                retVal = _UsdChangeType::kUpdate;
            }
        } else if (it->IsPropertyPath()) {
            // We have a bunch of UI properties to ignore. Especially anything that comes from UI
            // schemas.
            if (it->GetName().rfind("ui:", 0) == 0) {
                continue;
            }
            retVal = _UsdChangeType::kUpdate;
            for (const auto& entry : it.base()->second) {
                if (entry->flags.didChangeAttributeConnection) {
                    retVal = _UsdChangeType::kResync;
                    break;
                }
            }
        }
    }

    return retVal;
}

} // namespace

bool MayaUsdProxyShapeUpdateManager::CanIgnoreObjectsChanged(
    const UsdNotice::ObjectsChanged& notice)
{
    switch (_ClassifyUsdObjectsChanged(notice)) {
    case _UsdChangeType::kIgnored: return true;
    case _UsdChangeType::kResync: ++_UsdStageResyncCounter;
    // [[fallthrough]]; // We want that fallthrough to have the update always triggered.
    case _UsdChangeType::kUpdate: ++_UsdStageUpdateCounter; break;
    }

    return false;
}

MInt64 MayaUsdProxyShapeUpdateManager::GetUpdateCount() { return _UsdStageUpdateCounter; }

MInt64 MayaUsdProxyShapeUpdateManager::GetResyncCount() { return _UsdStageResyncCounter; }

PXR_NAMESPACE_CLOSE_SCOPE
