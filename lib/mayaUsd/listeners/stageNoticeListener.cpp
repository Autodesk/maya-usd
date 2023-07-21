//
// Copyright 2018 Pixar
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
#include "stageNoticeListener.h"

#include <pxr/base/tf/notice.h>
#include <pxr/base/tf/weakBase.h>
#include <pxr/base/vt/value.h>
#include <pxr/usd/sdf/listOp.h>
#include <pxr/usd/usd/notice.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/tokens.h>
#include <pxr/usd/usdShade/nodeGraph.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdUI/tokens.h>

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

/* virtual */
UsdMayaStageNoticeListener::~UsdMayaStageNoticeListener()
{
    if (_stageContentsChangedKey.IsValid()) {
        TfNotice::Revoke(_stageContentsChangedKey);
    }
    if (_stageObjectsChangedKey.IsValid()) {
        TfNotice::Revoke(_stageObjectsChangedKey);
    }
}

void UsdMayaStageNoticeListener::SetStage(const UsdStageWeakPtr& stage)
{
    _stage = stage;

    _UpdateStageContentsChangedRegistration();
}

void UsdMayaStageNoticeListener::SetStageContentsChangedCallback(
    const StageContentsChangedCallback& callback)
{
    _stageContentsChangedCallback = callback;

    _UpdateStageContentsChangedRegistration();
}

void UsdMayaStageNoticeListener::SetStageObjectsChangedCallback(
    const StageObjectsChangedCallback& callback)
{
    _stageObjectsChangedCallback = callback;

    _UpdateStageContentsChangedRegistration();
}

void UsdMayaStageNoticeListener::SetStageLayerMutingChangedCallback(
    const StageLayerMutingChangedCallback& callback)
{
    _stageLayerMutingChangedCallback = callback;

    _UpdateStageContentsChangedRegistration();
}

void UsdMayaStageNoticeListener::SetStageEditTargetChangedCallback(
    const StageEditTargetChangedCallback& callback)
{
    _stageEditTargetChangedCallback = callback;

    _UpdateStageContentsChangedRegistration();
}

void UsdMayaStageNoticeListener::_UpdateStageContentsChangedRegistration()
{
    if (_stage && _stageContentsChangedCallback) {
        // Register for notices if we're not already listening.
        if (!_stageContentsChangedKey.IsValid()) {
            _stageContentsChangedKey = TfNotice::Register(
                TfCreateWeakPtr(this), &UsdMayaStageNoticeListener::_OnStageContentsChanged);
        }
    } else {
        // Either the stage or the callback is invalid, so stop listening for
        // notices.
        if (_stageContentsChangedKey.IsValid()) {
            TfNotice::Revoke(_stageContentsChangedKey);
        }
    }

    if (_stage && _stageObjectsChangedCallback) {
        // Register for notices if we're not already listening.
        if (!_stageObjectsChangedKey.IsValid()) {
            _stageObjectsChangedKey = TfNotice::Register(
                TfCreateWeakPtr(this), &UsdMayaStageNoticeListener::_OnStageObjectsChanged, _stage);
        }
    } else {
        // Either the stage or the callback is invalid, so stop listening for
        // notices.
        if (_stageObjectsChangedKey.IsValid()) {
            TfNotice::Revoke(_stageObjectsChangedKey);
        }
    }

    if (_stage && _stageLayerMutingChangedCallback) {
        // Register for notices if we're not already listening.
        if (!_stageLayerMutingChangedKey.IsValid()) {
            _stageLayerMutingChangedKey = TfNotice::Register(
                TfCreateWeakPtr(this),
                &UsdMayaStageNoticeListener::_OnStageLayerMutingChanged,
                _stage);
        }
    } else {
        // Either the stage or the callback is invalid, so stop listening for
        // notices.
        if (_stageLayerMutingChangedKey.IsValid()) {
            TfNotice::Revoke(_stageLayerMutingChangedKey);
        }
    }

    if (_stage && _stageEditTargetChangedCallback) {
        // Register for notices if we're not already listening.
        if (!_stageEditTargetChangedKey.IsValid()) {
            _stageEditTargetChangedKey = TfNotice::Register(
                TfCreateWeakPtr(this),
                &UsdMayaStageNoticeListener::_OnStageEditTargetChanged,
                _stage);
        }
    } else {
        // Either the stage or the callback is invalid, so stop listening for
        // notices.
        if (_stageEditTargetChangedKey.IsValid()) {
            TfNotice::Revoke(_stageEditTargetChangedKey);
        }
    }
}

void UsdMayaStageNoticeListener::_OnStageContentsChanged(
    const UsdNotice::StageContentsChanged& notice) const
{
    if (notice.GetStage() == _stage && _stageContentsChangedCallback) {
        _stageContentsChangedCallback(notice);
    }
}

void UsdMayaStageNoticeListener::_OnStageObjectsChanged(
    const UsdNotice::ObjectsChanged& notice,
    const UsdStageWeakPtr&           sender) const
{
    if (notice.GetStage() == _stage && _stageObjectsChangedCallback) {
        _stageObjectsChangedCallback(notice);
    }
}

void UsdMayaStageNoticeListener::_OnStageLayerMutingChanged(
    const UsdNotice::LayerMutingChanged& notice) const
{
    if (notice.GetStage() == _stage && _stageLayerMutingChangedCallback) {
        _stageLayerMutingChangedCallback(notice);
    }
}

void UsdMayaStageNoticeListener::_OnStageEditTargetChanged(
    const UsdNotice::StageEditTargetChanged& notice) const
{
    if (notice.GetStage() == _stage && _stageEditTargetChangedCallback) {
        _stageEditTargetChangedCallback(notice);
    }
}

namespace {
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

bool _IsShadingPrim(const UsdPrim& prim)
{
    return prim && (prim.IsA<UsdShadeShader>() || prim.IsA<UsdShadeNodeGraph>());
}
} // namespace

UsdMayaStageNoticeListener::ChangeType
UsdMayaStageNoticeListener::ClassifyObjectsChanged(UsdNotice::ObjectsChanged const& notice)
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
            return resyncCount ? ChangeType::kResync : ChangeType::kIgnored;
        } else {
            return ChangeType::kResync;
        }
    }

    auto retVal = ChangeType::kIgnored;

    const PathRange pathsToUpdate = notice.GetChangedInfoOnlyPaths();
    for (PathRange::const_iterator it = pathsToUpdate.begin(); it != pathsToUpdate.end(); ++it) {
        if (it->IsAbsoluteRootOrPrimPath()) {
            const TfTokenVector changedFields = it.GetChangedFields();
            for (auto&& changedField : changedFields) {
                if (changedField == SdfFieldKeys->CustomData && it->IsPrimPath()
                    && _IsShadingPrim(notice.GetStage()->GetPrimAtPath(it->GetPrimPath()))) {
                    continue;
                }
                retVal = ChangeType::kUpdate;
                break;
            }
        } else if (it->IsPropertyPath()) {
            // We have a bunch of UI properties to ignore. Especially anything that comes from UI
            // schemas.
            if (it->GetName().rfind("ui:", 0) == 0) {
                continue;
            }
            retVal = ChangeType::kUpdate;
            for (const auto& entry : it.base()->second) {
                if (entry->flags.didChangeAttributeConnection) {
                    retVal = ChangeType::kResync;
                    break;
                }
            }
        }
    }

    return retVal;
}

PXR_NAMESPACE_CLOSE_SCOPE
