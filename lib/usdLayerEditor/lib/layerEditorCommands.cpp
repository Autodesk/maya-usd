//
// Copyright 2020 Autodesk
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

#include "LayerEditorCommands.h"

#include "layerEditorDCCFunctions.h"
#include "layerLocking.h"
#include "layerMuting.h"
#include "utilFileSystem.h"
#include "utilUI.h"

#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/pcp/layerStack.h>
#include <pxr/usd/usd/flattenUtils.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <usdUfe/ufe/Utils.h>
#include <usdUfe/undo/UsdUndoBlock.h>
#include <usdUfe/undo/UsdUndoManager.h>
#include <usdUfe/undo/UsdUndoableItem.h>
#include <usdUfe/utils/layers.h>
#include <usdUfe/utils/uiCallback.h>

#include <pxr/usd/usdUtils/stageCache.h>
#include <pxr/usd/usdUtils/stitch.h>

#include <ufe/globalSelection.h>
#include <ufe/observableSelection.h>

#include <algorithm>
#include <filesystem>
#include <set>
#include <unordered_map>
#include <utility>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
std::function<bool()>                              sAutoRetargetDisabled;
std::function<std::vector<UsdStageRefPtr>()>       sStagesProvider;
} // namespace

namespace UsdLayerEditor {

void BaseCmd::setAutoRetargetDisabledChecker(std::function<bool()> checker)
{
    sAutoRetargetDisabled = std::move(checker);
}

void BackupLayerBaseCmd::setStagesProvider(
    std::function<std::vector<UsdStageRefPtr>()> provider)
{
    sStagesProvider = std::move(provider);
}

void BaseCmd::holdOnPathIfDirty(const SdfLayerHandle& layer, const std::string& path)
{
    auto subLayerHandle = SdfLayer::FindRelativeToLayer(layer, path);
    if (subLayerHandle != nullptr) {
        if (subLayerHandle->IsDirty() || subLayerHandle->IsAnonymous()) {
            _subLayersRefs.push_back(subLayerHandle);
        }
        holdOntoSubLayers(subLayerHandle); // we'll need to hold onto children as well
    }
}

void BaseCmd::undo()
{
    // Signal command failure by throwing - as per UFE pattern.
    if (!undoIt(_layer)) {
        std::string msg = commandString() + " command undo failed.";
        throw std::runtime_error(msg);
    };
}

void BaseCmd::redo()
{
    // Signal command failure by throwing - as per UFE pattern.
    if (!doIt(_layer)) {
        std::string msg = commandString() + " command failed.";
        throw std::runtime_error(msg);
    };
}

// hold references to any anon or dirty sublayer
void BaseCmd::holdOntoSubLayers(const SdfLayerHandle& layer)
{
    const std::vector<std::string>& sublayers = layer->GetSubLayerPaths();
    for (auto path : sublayers) {
        holdOnPathIfDirty(layer, path);
    }
}

// Set the edit target to Session layer if no other layers are modifiable
void BaseCmd::updateEditTarget(const PXR_NS::UsdStageWeakPtr stage)
{
    if (sAutoRetargetDisabled && sAutoRetargetDisabled())
        return;

    if (!stage)
        return;

    // Edit-forwarding integrations manage their own edit target: when forwarding is
    // active they keep the stage edit target on the session layer and redirect the
    // (possibly locked) fallback target. In that case skip the normal auto-targeting.
    if (handleEFEditTargetUpdate(PXR_NS::UsdStageRefPtr(stage)))
        return;

    if (stage->GetEditTarget().GetLayer() == stage->GetSessionLayer())
        return;

    // If the currently targeted layer isn't locked, we don't need to change it.
    if (!isLayerLocked(stage->GetEditTarget().GetLayer()))
        return;

    // If there are no target-able layers, we set the target to session layer.
    std::string errMsg;
    if (!UsdUfe::isAnyLayerModifiable(stage, &errMsg)) {

        TF_RUNTIME_ERROR("%s", errMsg.c_str());
        stage->SetEditTarget(stage->GetSessionLayer());
    }
}

bool BackupLayerBaseCmd::doIt(const SdfLayerHandle& layer)
{
    backupLayer(layer);

    // using reload will correctly reset the dirty bit
    holdOntoSubLayers(layer);

    if (_cmdId == CmdId::kDiscardEdit) {
        layer->Reload();
    } else if (_cmdId == CmdId::kClearLayer) {
        layer->Clear();
    } else if (_cmdId == CmdId::kFlattenLayer) {
        // Create a temp stage to get a PcpLayerStack with this layer as the root.
        PXR_NS::UsdStageRefPtr tempStage = PXR_NS::UsdStage::Open(layer);
        if (!tempStage) {
            UIUtils::displayError("Failed to open stage for layer");
            return false;
        }

        // Get the PcpLayerStackRefPtr to be used in the flatten method.
        PXR_NS::PcpLayerStackRefPtr layerStack;
        PXR_NS::UsdPrim             rootPrim = tempStage->GetPseudoRoot();
        if (rootPrim) {
            PXR_NS::PcpPrimIndex primIndex = rootPrim.GetPrimIndex();
            if (primIndex.IsValid()) {
                PXR_NS::PcpNodeRef rootNode = primIndex.GetRootNode();
                if (rootNode) {
                    layerStack = rootNode.GetLayerStack();
                }
            }
        }

        if (!layerStack) {
            UIUtils::displayError("Cannot flatten layer: could not determine layer stack");
            return false;
        }

        PXR_NS::SdfLayerRefPtr flattenedLayer = PXR_NS::UsdFlattenLayerStack(layerStack);
        if (!flattenedLayer) {
            UIUtils::displayError("Failed to flatten layer stack");
            return false;
        }

        layer->TransferContent(flattenedLayer);
    }

    // Note: backup the edit targets after the layer is cleared because we use
    //       the fact that a stage edit target is now invalid to decide to backup
    //       that edit target.
    backupEditTargets(layer);

    return true;
}

bool BackupLayerBaseCmd::undoIt(const SdfLayerHandle& layer)
{
    restoreLayer(layer);

    // Note: restore edit targets after the layers are restored so that the backup
    //       edit targets are now valid.
    restoreEditTargets();

    releaseSubLayers();

    return true;
}

void BackupLayerBaseCmd::backupLayer(const SdfLayerHandle& layer)
{
    if (!layer)
        return;

    if (layer->IsDirty() || _cmdId != CmdId::kDiscardEdit) {
        _backupLayer = SdfLayer::CreateAnonymous();
        _backupLayer->TransferContent(layer);
    }
}

void BackupLayerBaseCmd::restoreLayer(const SdfLayerHandle& layer)
{
    if (!layer)
        return;

    if (_backupLayer) {
        layer->TransferContent(_backupLayer);
        _backupLayer = nullptr;
    } else {
        layer->Reload();
    }
}

void BackupLayerBaseCmd::backupEditTargets(const SdfLayerHandle& layer)
{
    _editTargetBackups.clear();

    if (!layer)
        return;

    const std::vector<UsdStageRefPtr> stages = sStagesProvider
        ? sStagesProvider()
        : UsdUtilsStageCache::Get().GetAllStages();

    for (const PXR_NS::UsdStageRefPtr& stage : stages) {
        if (!stage)
            continue;
        const PXR_NS::UsdEditTarget target = stage->GetEditTarget();
        // Note: this is the check that UsdStage::SetTargetLayer would do
        //       which is how we would detect that the edit target is now
        //       invalid. Unfortunately, there is no USD function to check
        //       if an edit target is valid outside of trying to set it as
        //       the edit target, but we would not want to set it. (Also,
        //       knowing if the stage checks that the edit target is already
        //       set to the same target before validating it is an implementation
        //       detail that we would raher not rely on.)
        if (stage->HasLocalLayer(target.GetLayer()))
            continue;
        _editTargetBackups[stage] = target;

        // Set a valid target. The only layer we can count on is the root
        // layer, so set the target to that.
        stage->SetEditTarget(stage->GetRootLayer());
    }
}

void BackupLayerBaseCmd::restoreEditTargets()
{
    for (const auto& weakStageAndTarget : _editTargetBackups) {
        const PXR_NS::UsdStageRefPtr stage = weakStageAndTarget.first;
        if (!stage)
            continue;

        PXR_NS::UsdEditTarget target = weakStageAndTarget.second;
        stage->SetEditTarget(target);
    }
}

bool LockLayerCmd::doIt(const SdfLayerHandle& layer)
{
    auto stage = getStage();
    if (!stage)
        return false;

    std::set<PXR_NS::SdfLayerRefPtr> layersToUpdate;
    if (_includeSublayers) {
        // If _includeSublayers is True, we attempt to refresh the system lock status of all
        // layers under the given layer. This is specially useful when reloading a stage.
        bool includeTopLayer = true;
        layersToUpdate = UsdUfe::getAllSublayerRefs(layer, includeTopLayer);
    } else {
        layersToUpdate.insert(layer);
    }

    for (auto layerIt : layersToUpdate) {
        if (isLayerLocked(layerIt)) {
            _previousStates.push_back(LayerLockType::LayerLock_Locked);
        } else if (isLayerSystemLocked(layerIt)) {
            _previousStates.push_back(LayerLockType::LayerLock_SystemLocked);
        } else {
            _previousStates.push_back(LayerLockType::LayerLock_Unlocked);
        }
        _layers.push_back(layerIt);
    }

    // Execute lock commands
    for (size_t layerIndex = 0; layerIndex < _layers.size(); layerIndex++) {
        auto curLayer = _layers[layerIndex];
        // Note: per design, we refuse to affect the lock status of system-locked
        //       sub-layers from the UI. The skip-system-locked flag is used for that.
        if (_skipSystemLockedLayers) {
            if (curLayer != layer) {
                if (_lockType != LayerLockType::LayerLock_SystemLocked) {
                    if (isLayerSystemLocked(curLayer)) {
                        continue;
                    }
                }
            }
        }

        const auto dccObjectPath = UsdUfe::stagePath(stage).string();
        lockLayer(dccObjectPath, curLayer, _lockType, true);
    }

    if (_updateEditTarget) {
        updateEditTarget(stage);
    }

    return true;
}

bool LockLayerCmd::undoIt(const SdfLayerHandle& layer)
{
    auto stage = getStage();
    if (!stage)
        return false;

    if (_layers.size() != _previousStates.size()) {
        return false;
    }

    const auto dccObjectPath = UsdUfe::stagePath(stage).string();

    // Execute lock commands
    for (size_t layerIndex = 0; layerIndex < _layers.size(); layerIndex++) {
        // Note: the undo of system-locked is unlocked by design.
        if (_lockType == LayerLockType::LayerLock_SystemLocked) {
            lockLayer(dccObjectPath, _layers[layerIndex], LayerLockType::LayerLock_Unlocked, true);
        } else {
            lockLayer(dccObjectPath, _layers[layerIndex], _previousStates[layerIndex], true);
        }
    }

    if (_updateEditTarget) {
        updateEditTarget(stage);
    }

    return true;
}

UsdStageWeakPtr LockLayerCmd::getStage() { return _stage; }

bool MuteLayerCmd::doIt(const SdfLayerHandle& layer)
{
    auto stage = getStage();
    if (!stage)
        return false;
    if (_muteIt) {
        // We prefer not holding to pointers needlessly, but we need to hold on
        // to the muted layer. OpenUSD lets go of muted layers, so anonymous
        // layers and any dirty children would be lost if not explicitly held on.
        // This is done before really muting the layer to ensure no sublayer is
        // gone after the mute change.
        addMutedLayer(layer);

        // Muting a layer will cause all scene items under the proxy shape
        // to be stale.
        saveSelection();
        stage->MuteLayer(layer->GetIdentifier());
    } else {
        stage->UnmuteLayer(layer->GetIdentifier());

        // We can release the now unmuted layer.
        removeMutedLayer(layer);

        restoreSelection();
    }

    updateEditTarget(stage);

    return true;
}

bool MuteLayerCmd::undoIt(const SdfLayerHandle& layer)
{
    auto stage = getStage();
    if (!stage)
        return false;
    if (_muteIt) {
        stage->UnmuteLayer(layer->GetIdentifier());

        // We can release the now unmuted layer.
        removeMutedLayer(layer);

        restoreSelection();
    } else {
        // Hold the layer before re-muting it, mirroring doIt (so no sublayer is
        // gone after the mute change).
        addMutedLayer(layer);

        // Muting a layer will cause all scene items under the proxy shape
        // to be stale.
        saveSelection();
        stage->MuteLayer(layer->GetIdentifier());
    }

    updateEditTarget(stage);

    return true;
}

UsdStageWeakPtr MuteLayerCmd::getStage() { return _stage; }

void MuteLayerCmd::saveSelection()
{
    // Make a copy of the global selection, to restore it on unmute.
    auto globalSn = Ufe::GlobalSelection::get();
    _savedSn.replaceWith(*globalSn);
    // Filter the global selection, removing items below our DCC object.
    auto path = UsdUfe::stagePath(_stage);
    globalSn->replaceWith(UsdUfe::removeDescendants(_savedSn, path));
}

void MuteLayerCmd::restoreSelection()
{
    // Restore the saved selection to the global selection.
    auto path = UsdUfe::stagePath(_stage);
    auto globalSn = Ufe::GlobalSelection::get();
    globalSn->replaceWith(UsdUfe::recreateDescendants(_savedSn, path));
}

InsertRemoveSubPathBaseCmd::InsertRemoveSubPathBaseCmd(
    CmdId                      id,
    const pxr::UsdStageRefPtr& stage,
    const pxr::SdfLayerRefPtr& layer,
    const std::string&         subpath,
    int                        index)
    : BaseCmd(id, layer)
    , _stage(stage)
    , _subPath(subpath)
    , _index(index)
{
}

bool InsertRemoveSubPathBaseCmd::doIt(const pxr::SdfLayerHandle& layer)
{
    if (_cmdId == CmdId::kInsert || _cmdId == CmdId::kAddAnonLayer) {
        if (_index == -1) {
            _index = (int)layer->GetNumSubLayerPaths();
        }
        if (_index != 0) {
            if (!validateAndReportIndex(layer, _index, (int)layer->GetNumSubLayerPaths() + 1)) {
                return false;
            }
        }

        // According to USD codebase, we should always call SdfLayer::InsertSubLayerPath()
        // with a layer's identifier. So, if the layer exists, override _subPath with the
        // identifier in case this command was called with a filesystem path. Otherwise,
        // adding the layer with the filesystem path can cause issue when muting the layer
        // on Windows if the path is absolute and start with a capital drive letter.
        //
        // Note: It's possible that SdfLayer::FindOrOpen() fail because we
        //       allow user to add layer that does not exists.
        auto layerToAdd = SdfLayer::FindOrOpen(_subPath);
        if (layerToAdd) {
            _subPath = layerToAdd->GetIdentifier();
        }

        layer->InsertSubLayerPath(_subPath, _index);
        TF_VERIFY(
            (static_cast<size_t>(_index) < layer->GetSubLayerPaths().size())
            && layer->GetSubLayerPaths()[_index] == _subPath);
    } else {
        TF_VERIFY(_cmdId == CmdId::kRemove);

        // If we build the remove layer command using a path - find matching sublayer index.
        if (_index == -1) {
            _index = layer->GetSubLayerPaths().Find(_subPath);
        }

        if (!validateAndReportIndex(layer, _index, (int)layer->GetNumSubLayerPaths())) {
            return false;
        }
        saveSelection();

        // If we build the remove layer command using an index - find matching sublayer path.
        if (_subPath.empty()) {
            _subPath = layer->GetSubLayerPaths()[_index];    
        }
        
        holdOnPathIfDirty(layer, _subPath);

        // if the current edit target is the layer to remove or
        // a sublayer of the layer to remove,
        // set the root layer as the current edit target
        auto layerToRemove = SdfLayer::FindRelativeToLayer(layer, _subPath);
        auto currentTarget = _stage->GetEditTarget().GetLayer();

        // Helper function to find if a layer is in the
        // hierarchy of another layer
        //
        // rootLayer: The root layer of the hierarchy
        // layer: The layer to find
        // ignore : Optional layer used has the root of a hierarchy that
        //          we don't want to check in.
        // ignoreSubPath : Optional subpath used whith ignore layer.
        auto isInHierarchy = [](const SdfLayerHandle& rootLayer,
                                const SdfLayerHandle& layer,
                                const SdfLayerHandle* ignore = nullptr,
                                const std::string*    ignoreSubPath = nullptr) {
            // Impl used for recursive call
            auto isInHierarchyImpl = [](const SdfLayerHandle& rootLayer,
                                        const SdfLayerHandle& layer,
                                        const SdfLayerHandle* ignore,
                                        const std::string*    ignoreSubPath,
                                        auto&                 implRef) {
                if (!rootLayer || !layer)
                    return false;

                if (rootLayer->GetIdentifier() == layer->GetIdentifier())
                    return true;

                const auto subLayerPaths = rootLayer->GetSubLayerPaths();
                for (const auto& subLayerPath : subLayerPaths) {

                    if (ignore && ignoreSubPath
                        && (*ignore)->GetIdentifier() == rootLayer->GetIdentifier()
                        && *ignoreSubPath == subLayerPath)
                        continue;

                    const auto subLayer = SdfLayer::FindRelativeToLayer(rootLayer, subLayerPath);
                    if (implRef(subLayer, layer, ignore, ignoreSubPath, implRef))
                        return true;
                }
                return false;
            };
            return isInHierarchyImpl(rootLayer, layer, ignore, ignoreSubPath, isInHierarchyImpl);
        };

        if (isInHierarchy(layerToRemove, currentTarget)) {
            // The current edit layer is in the hierarchy of the layer to remove,
            // now we need to be sure the edit target layer is not also a sublayer
            // of another layer in the stage.
            if (!isInHierarchy(_stage->GetRootLayer(), currentTarget, &layer, &_subPath)) {
                _editTargetPath = currentTarget->GetIdentifier();
                _stage->SetEditTarget(_stage->GetRootLayer());
            }
        }

        layer->RemoveSubLayerPath(_index);
    }
    return true;
}

bool InsertRemoveSubPathBaseCmd::undoIt(const pxr::SdfLayerHandle& layer)
{
    if (_cmdId == CmdId::kInsert || _cmdId == CmdId::kAddAnonLayer) {
        auto index = _index;
        if (index == -1) {
            index = static_cast<int>(layer->GetNumSubLayerPaths() - 1);
        }
        if (validateUndoIndex(layer, _index)) {
            TF_VERIFY(layer->GetSubLayerPaths()[index] == _subPath);
            layer->RemoveSubLayerPath(index);
        } else {
            return false;
        }
    } else {
        TF_VERIFY(_index != -1);
        if (validateUndoIndex(layer, _index)) {
            layer->InsertSubLayerPath(_subPath, _index);

            // if the removed layer was the edit target,
            // set it back to the current edit target
            if (!_editTargetPath.empty()) {
                auto subLayerHandle = SdfLayer::FindRelativeToLayer(layer, _editTargetPath);
                _stage->SetEditTarget(subLayerHandle);
            }
        } else {
            return false;
        }
        restoreSelection();
    }
    return true;
}

bool InsertRemoveSubPathBaseCmd::validateUndoIndex(const pxr::SdfLayerHandle& layer, int index)
{ // allow re-inserting at the last index + 1, but -1 should have been changed to 0
    return !(index < 0 || index > (int)layer->GetNumSubLayerPaths());
}

bool InsertRemoveSubPathBaseCmd::validateAndReportIndex(
    const pxr::SdfLayerHandle& layer,
    int                        index,
    int                        maxIndex)
{
    if (index < 0 || index >= maxIndex) {
        std::string message = std::string("Index ") + std::to_string(index)
            + std::string(" out-of-bound for ") + layer->GetIdentifier();
        UIUtils::displayError(message.c_str());
        return false;
    } else {
        return true;
    }
}

void InsertRemoveSubPathBaseCmd::saveSelection()
{
    // Make a copy of the global selection, to restore it on unlock.
    auto globalSn = Ufe::GlobalSelection::get();
    _savedSn.replaceWith(*globalSn);
    // Filter the global selection, removing items below our DCC object.
    auto path = UsdUfe::stagePath(_stage);
    globalSn->replaceWith(UsdUfe::removeDescendants(_savedSn, path));
}

void InsertRemoveSubPathBaseCmd::restoreSelection()
{
    // Restore the saved selection to the global selection.  If a saved
    // selection item started with the proxy shape path, re-create it.
    auto globalSn = Ufe::GlobalSelection::get();
    auto path = UsdUfe::stagePath(_stage);
    globalSn->replaceWith(UsdUfe::recreateDescendants(_savedSn, path));
}

bool ReplaceSubPathCmd::doIt(const SdfLayerHandle& layer)
{
    auto proxy = layer->GetSubLayerPaths();
    if (proxy.Find(_oldPath) == static_cast<size_t>(-1)) {
        std::string message = std::string("path ") + _oldPath
            + std::string(" not found on layer ") + layer->GetIdentifier();
        UIUtils::displayError(message.c_str());
        return false;
    }
    holdOnPathIfDirty(layer, _oldPath);
    proxy.Replace(_oldPath, _newPath);
    return true;
}

bool ReplaceSubPathCmd::undoIt(const SdfLayerHandle& layer)
{
    auto proxy = layer->GetSubLayerPaths();
    proxy.Replace(_newPath, _oldPath);
    releaseSubLayers();
    holdOnPathIfDirty(layer, _newPath);
    return true;
}

bool MoveSubPathCmd::doIt(const pxr::SdfLayerHandle& layer)
{
    auto proxy = layer->GetSubLayerPaths();
    auto subPathIndex = proxy.Find(_subPath);
    if (subPathIndex == size_t(-1)) {
        TF_RUNTIME_ERROR(
            "path %s not found on layer %s",
            _subPath.c_str(),
            layer->GetIdentifier().c_str());
        return false;
    }
    _oldIndex = static_cast<int>(subPathIndex);

    std::string newPath = _subPath;

    if (layer->GetIdentifier() == _newParent->GetIdentifier()) {
        // Same-parent reorder: bounds-check against current count (before removal)
        if (_newIndex > static_cast<int>(layer->GetNumSubLayerPaths()) - 1) {
            TF_RUNTIME_ERROR(
                "Index %d out-of-bound for %s",
                _newIndex,
                layer->GetIdentifier().c_str());
            return false;
        }
    } else {
        // Cross-parent move: append is allowed, so bound is GetNumSubLayerPaths()
        if (_newIndex > static_cast<int>(_newParent->GetNumSubLayerPaths())) {
            TF_RUNTIME_ERROR(
                "Index %d out-of-bound for %s",
                _newIndex,
                _newParent->GetIdentifier().c_str());
            return false;
        }

        // Reparent relative file paths
        namespace fs = std::filesystem;
        fs::path filePath(_subPath);
        bool     needsRepathing = !SdfLayer::IsAnonymousLayerIdentifier(_subPath)
            && filePath.is_relative() && !layer->GetRealPath().empty()
            && !_newParent->GetRealPath().empty();

        if (needsRepathing) {
            auto        oldLayerDir = fs::path(layer->GetRealPath()).remove_filename();
            auto        newLayerDir = fs::path(_newParent->GetRealPath()).remove_filename();
            std::string absolutePath
                = (oldLayerDir / filePath).lexically_normal().generic_string();
            auto result = FileSystem::makePathRelativeTo(
                absolutePath, newLayerDir.lexically_normal().generic_string());
            if (result.second) {
                newPath = result.first;
            } else {
                newPath = absolutePath;
                TF_WARN(
                    "File name (%s) cannot be resolved as relative to layer %s, using "
                    "absolute path.",
                    absolutePath.c_str(),
                    _newParent->GetIdentifier().c_str());
            }
        }

        if (_newParent->GetSubLayerPaths().Find(newPath) != size_t(-1)) {
            TF_RUNTIME_ERROR(
                "SubPath %s already exists in layer %s",
                newPath.c_str(),
                _newParent->GetIdentifier().c_str());
            return false;
        }
    }

    _newPath = newPath;
    layer->RemoveSubLayerPath(_oldIndex);
    _newParent->InsertSubLayerPath(_newPath, _newIndex);
    return true;
}

bool MoveSubPathCmd::undoIt(const pxr::SdfLayerHandle& layer)
{
    _newParent->RemoveSubLayerPath(_newIndex);
    layer->InsertSubLayerPath(_subPath, _oldIndex);
    return true;
}

bool RefreshSystemLockLayerCmd::doIt(const pxr::SdfLayerHandle& layer)
{
    if (!_stage) {
        return false;
    }

    if (_refreshSubLayers) {
        // If refreshSubLayers is True, we attempt to refresh the system lock status of all
        // layers under the given layer. This is specially useful when reloading a stage.
        bool includeTopLayer = true;
        auto allLayers = UsdUfe::getAllSublayerRefs(layer, includeTopLayer);
        for (auto layerIt : allLayers) {
            _refreshLayerSystemLock(layerIt);
        }
    } else {
        // Only check and refresh the system lock status of the current layer.
        _refreshLayerSystemLock(layer);
    }

    // Execute lock commands
    for (size_t layerIndex = 0; layerIndex < _layers.size(); layerIndex++) {
        if (!_lockCommands[layerIndex]->doIt(_layers[layerIndex])) {
            return false;
        }
    }

    if (!_layers.empty()) {
        _notifySystemLockIsRefreshed();

        // Finally update edit target after layer locks were changed
        // by the command or a callback.
        updateEditTarget(_stage);
    }

    return true;
}

bool RefreshSystemLockLayerCmd::undoIt(const pxr::SdfLayerHandle& layer)
{
    if (!_stage) {
        return false;
    }

    // Execute lock commands
    for (size_t layerIndex = 0; layerIndex < _layers.size(); layerIndex++) {
        if (!_lockCommands[layerIndex]->undoIt(_layers[layerIndex])) {
            return false;
        }
    }

    if (!_layers.empty()) {
        _notifySystemLockIsRefreshed();

        // Finally update edit target after layer locks were changed
        // by the command or a callback.
        updateEditTarget(_stage);
    }

    return true;
}

std::string RefreshSystemLockLayerCmd::_quote(const std::string& string)
{
    return std::string(" \"") + string + std::string("\"");
}

void RefreshSystemLockLayerCmd::addCallbackContext(
    const std::string& key, const pxr::VtValue& value)
{
    _extraCallbackContext[key] = value;
}

void RefreshSystemLockLayerCmd::_refreshLayerSystemLock(const pxr::SdfLayerHandle& usdLayer)
{
    // Anonymous layers do not need to be checked.
    if (usdLayer && !usdLayer->IsAnonymous()) {
        // Check if the layer's write permissions have changed.
        std::string assetPath = usdLayer->GetResolvedPath();
        std::replace(assetPath.begin(), assetPath.end(), '\\', '/');

        if (!assetPath.empty()) {

            auto writeAccess = UsdLayerEditor::FileSystem::checkWriteAccess(assetPath);

            if (writeAccess && isLayerSystemLocked(usdLayer)) {
                // If the file has write permissions and the layer is currently
                // system-locked: Unlock the layer

                // Create the lock command
                auto cmd
                    = std::make_shared<LockLayerCmd>(_stage, usdLayer, LayerLock_Unlocked, false);
                // Edit target will be updated once at the end of the refresh command.
                cmd->SetUpdateEditTarget(false);

                // Add the lock command and its parameter to be executed
                _lockCommands.push_back(std::move(cmd));
                _layers.push_back(usdLayer);
            } else if (!writeAccess && !isLayerSystemLocked(usdLayer)) {
                // If the file doesn't have write permissions and the layer is currently not
                // system-locked: System-lock the layer

                // Create the lock command
                auto cmd = std::make_shared<LockLayerCmd>(
                    _stage, usdLayer, LayerLock_SystemLocked, false);
                // Edit target will be updated once at the end of the refresh command.
                cmd->SetUpdateEditTarget(false);

                // Add the lock command and its parameter to be executed
                _lockCommands.push_back(std::move(cmd));
                _layers.push_back(usdLayer);
            }
        }
    }
}

void RefreshSystemLockLayerCmd::_notifySystemLockIsRefreshed()
{
    if (!UsdUfe::isUICallbackRegistered(TfToken("onRefreshSystemLock"))) {
        return;
    }

    PXR_NS::VtDictionary callbackContext;
    callbackContext["objectPath"] = PXR_NS::VtValue(UsdUfe::stagePath(_stage).string().c_str());
    for (const auto& entry : _extraCallbackContext) {
        callbackContext[entry.first] = entry.second;
    }
    PXR_NS::VtDictionary callbackData;

    std::vector<std::string> affectedLayers;
    affectedLayers.reserve(_layers.size());
    for (size_t layerIndex = 0; layerIndex < _layers.size(); layerIndex++) {
        affectedLayers.push_back(_layers[layerIndex]->GetIdentifier());
    }

    VtStringArray lockedArray(affectedLayers.begin(), affectedLayers.end());
    callbackData["affectedLayerIds"] = lockedArray;

    UsdUfe::triggerUICallback(TfToken("onRefreshSystemLock"), callbackContext, callbackData);
}

bool StitchLayersCmd::doIt(const SdfLayerHandle& /*layer*/)
{
    if (_layerIdentifiersByStrength.empty())
        return true;

    if (!_stage) {
        TF_RUNTIME_ERROR("Cannot stitch layers: no valid stage");
        return false;
    }

    const SdfLayerHandleVector stageLayers = _stage->GetLayerStack();

    // Sort the selected layers by their strength (strongest first).
    std::unordered_map<std::string, size_t> layerStrengthMap;
    layerStrengthMap.reserve(stageLayers.size());
    for (size_t i = 0; i < stageLayers.size(); ++i)
        layerStrengthMap[stageLayers[i]->GetIdentifier()] = i;

    std::sort(
        _layerIdentifiersByStrength.begin(),
        _layerIdentifiersByStrength.end(),
        [&layerStrengthMap](const std::string& a, const std::string& b) {
            const auto itA = layerStrengthMap.find(a);
            const auto itB = layerStrengthMap.find(b);
            if (itA == layerStrengthMap.end()) {
                TF_WARN("Layer '%s' not found in stage layer stack", a.c_str());
                return false;
            }
            if (itB == layerStrengthMap.end()) {
                TF_WARN("Layer '%s' not found in stage layer stack", b.c_str());
                return true;
            }
            return itA->second < itB->second;
        });

    // Validate and collect all problems before modifying anything.
    bool hasProblems = false;

    SdfLayerHandleVector layersByStrength;
    layersByStrength.reserve(_layerIdentifiersByStrength.size());
    for (const auto& layerIdentifier : _layerIdentifiersByStrength) {
        auto foundLayer = SdfLayer::FindOrOpen(layerIdentifier);
        if (!foundLayer) {
            TF_RUNTIME_ERROR("Cannot find layer: %s", layerIdentifier.c_str());
            return false;
        }
        if (!foundLayer->PermissionToEdit()) {
            TF_WARN(
                "Cannot update layer '%s' because it is locked.",
                foundLayer->GetDisplayName().c_str());
            hasProblems = true;
        }
        layersByStrength.push_back(foundLayer);
    }

    const SdfLayerHandle strongestLayer = layersByStrength[0];

    std::map<std::string, std::pair<SdfLayerHandle, std::string>> parentInfoByLayer;
    for (const auto& potentialParent : stageLayers) {
        const std::vector<std::string>& subLayerPaths = potentialParent->GetSubLayerPaths();
        for (size_t i = 0; i < subLayerPaths.size(); ++i) {
            auto subLayer = SdfLayer::FindRelativeToLayer(potentialParent, subLayerPaths[i]);
            if (subLayer) {
                parentInfoByLayer[subLayer->GetIdentifier()]
                    = std::make_pair(potentialParent, subLayerPaths[i]);
            }
        }
    }

    // Multiple selected weak layers may share the same parent, so batch removals by parent
    // to avoid calling SetSubLayerPaths more than once per parent layer.
    std::map<std::string, std::vector<std::string>> removalsByParent;
    for (size_t i = 1; i < layersByStrength.size(); ++i) {
        const SdfLayerHandle& weakLayer = layersByStrength[i];
        const std::string     weakLayerId = weakLayer->GetIdentifier();

        const auto& it = parentInfoByLayer.find(weakLayerId);
        if (it == parentInfoByLayer.end()) {
            TF_WARN("Could not find parent for layer: %s", weakLayer->GetDisplayName().c_str());
            hasProblems = true;
            continue;
        }

        const SdfLayerHandle& parentLayer = it->second.first;
        if (!parentLayer->PermissionToEdit()) {
            TF_WARN(
                "Cannot update layer '%s' because its parent '%s' is locked.",
                weakLayer->GetDisplayName().c_str(),
                parentLayer->GetDisplayName().c_str());
            hasProblems = true;
            continue;
        }

        removalsByParent[parentLayer->GetIdentifier()].push_back(it->second.second);
    }

    if (hasProblems)
        return false;

    holdOntoSubLayers(strongestLayer);

    // Keep a hold of references for all selected layers, needed for undo().
    for (size_t i = 1; i < layersByStrength.size(); ++i)
        _subLayersRefs.push_back(layersByStrength[i]);

    UsdUfe::UsdUndoManager::instance().trackLayerStates(strongestLayer);
    for (size_t i = 1; i < layersByStrength.size(); ++i)
        UsdUfe::UsdUndoManager::instance().trackLayerStates(layersByStrength[i]);
    for (const auto& entry : removalsByParent) {
        auto parentLayer = SdfLayer::Find(entry.first);
        if (parentLayer) {
            UsdUfe::UsdUndoManager::instance().trackLayerStates(parentLayer);
        }
    }

    {
        UsdUfe::UsdUndoBlock undoBlock(&_undoItem);

        std::vector<std::vector<std::string>> movedSubLayers;
        movedSubLayers.reserve(layersByStrength.size() - 1);
        for (size_t i = 1; i < layersByStrength.size(); ++i) {
            const SdfLayerHandle& weakLayer = layersByStrength[i];
            movedSubLayers.push_back(weakLayer->GetSubLayerPaths());
            UsdUtilsStitchLayers(strongestLayer, weakLayer);
        }

        // Add all collected subLayers to the strongest layer, preventing duplicates.
        // Non-selected weak subLayers are added to the end of the subPathList of the
        // strongestLayer. Note: This means there are cases where relative strength is not fully
        // preserved.
        auto strongLayerSubLayers = strongestLayer->GetSubLayerPaths();

        // Creates a set of the added subLayers, prevent duplicates.
        std::set<std::string> addedSublayerIds;
        for (const auto path : strongLayerSubLayers) {
            const auto existingLayer = SdfLayer::FindRelativeToLayer(strongestLayer, path);
            if (existingLayer) {
                addedSublayerIds.insert(existingLayer->GetIdentifier());
            }
        }

        // Adds any moved sub layers to the strong layers sub layers to prevent layers from
        // being lost when the weak layer is deleted.
        for (const auto& subLayerList : movedSubLayers) {
            for (const auto& subLayerPath : subLayerList) {
                const auto subLayer
                    = SdfLayer::FindRelativeToLayer(strongestLayer, subLayerPath);
                if (subLayer
                    && addedSublayerIds.find(subLayer->GetIdentifier())
                        == addedSublayerIds.end()) {
                    strongLayerSubLayers.push_back(subLayerPath);
                    addedSublayerIds.insert(subLayer->GetIdentifier());
                }
            }
        }

        // Remove any merged weak layers from the sublayer list before setting, to prevent
        // them from being both stitched (merged) and referenced as subLayers.
        for (size_t i = 1; i < layersByStrength.size(); ++i) {
            const std::string weakLayerId = layersByStrength[i]->GetIdentifier();
            const auto        it = std::find(
                strongLayerSubLayers.begin(), strongLayerSubLayers.end(), weakLayerId);
            if (it != strongLayerSubLayers.end())
                strongLayerSubLayers.erase(it);
        }

        strongestLayer->SetSubLayerPaths(strongLayerSubLayers);

        // Removes the selected weak layers from their parents.
        // All parents were validated as editable before reaching this point.
        for (auto& entry : removalsByParent) {
            const auto parentLayer = SdfLayer::Find(entry.first);
            if (!parentLayer)
                continue;

            auto subLayerPaths = parentLayer->GetSubLayerPaths();
            for (const auto& pathToRemove : entry.second) {
                auto it = std::find(subLayerPaths.begin(), subLayerPaths.end(), pathToRemove);
                if (it != subLayerPaths.end())
                    subLayerPaths.erase(it);
            }
            parentLayer->SetSubLayerPaths(subLayerPaths);
        }
    }

    backupEditTargets(strongestLayer);

    return true;
}

bool StitchLayersCmd::undoIt(const SdfLayerHandle& /*layer*/)
{
    _undoItem.undo();

    restoreEditTargets();
    releaseSubLayers();

    return true;
}

} // namespace UsdLayerEditor