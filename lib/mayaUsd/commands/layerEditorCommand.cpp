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

#include "layerEditorCommand.h"

#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/utils/layerLocking.h>
#include <mayaUsd/utils/layerMuting.h>
#include <mayaUsd/utils/layers.h>
#include <mayaUsd/utils/query.h>
#include <mayaUsd/utils/stageCache.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilFileSystem.h>

#include <usdUfe/ufe/Utils.h>
#include <usdUfe/utils/uiCallback.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/envSetting.h>

#include <maya/MArgList.h>
#include <maya/MArgParser.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MSyntax.h>
#include <ufe/globalSelection.h>
#include <ufe/observableSelection.h>

#include <ghc/filesystem.hpp>

#include <cstddef>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
const char kInsertSubPathFlag[] = "is";
const char kInsertSubPathFlagL[] = "insertSubPath";
const char kRemoveSubPathFlag[] = "rs";
const char kRemoveSubPathFlagL[] = "removeSubPath";
const char kReplaceSubPathFlag[] = "rp";
const char kReplaceSubPathFlagL[] = "replaceSubPath";
const char kMoveSubPathFlag[] = "mv";
const char kMoveSubPathFlagL[] = "moveSubPath";
const char kDiscardEditsFlag[] = "de";
const char kDiscardEditsFlagL[] = "discardEdits";
const char kClearLayerFlag[] = "cl";
const char kClearLayerFlagL[] = "clear";
const char kAddAnonSublayerFlag[] = "aa";
const char kAddAnonSublayerFlagL[] = "addAnonymous";
const char kMuteLayerFlag[] = "mt";
const char kMuteLayerFlagL[] = "muteLayer";
const char kLockLayerFlag[] = "lk";
const char kLockLayerFlagL[] = "lockLayer";
const char kSkipSystemLockedFlag[] = "ssl";
const char kSkipSystemLockedFlagL[] = "skipSystemLocked";
const char kRefreshSystemLockFlag[] = "rl";
const char kRefreshSystemLockFlagL[] = "refreshSystemLock";

// Disables updateEditTarget's functionality is set.
// Areas that will be affected are:
// - Mute layer
// - Lock layer
// - System lock layer
TF_DEFINE_ENV_SETTING(
    MAYAUSD_LAYEREDITOR_DISABLE_AUTOTARGET,
    false,
    "When set, disables auto retargeting of layers based on the file and permission status.");
} // namespace

namespace MAYAUSD_NS_DEF {

namespace Impl {

enum class CmdId
{
    kInsert,
    kRemove,
    kMove,
    kReplace,
    kDiscardEdit,
    kClearLayer,
    kAddAnonLayer,
    kMuteLayer,
    kLockLayer,
    kRefreshSystemLock
};

class BaseCmd
{
public:
    BaseCmd(CmdId id)
        : _cmdId(id)
    {
    }
    virtual ~BaseCmd() { }
    virtual bool doIt(SdfLayerHandle layer) = 0;
    virtual bool undoIt(SdfLayerHandle layer) = 0;

    std::string _cmdResult; // set if the command returns something

protected:
    CmdId _cmdId;
    // we need to hold on to dirty sublayers if we remove them
    std::vector<PXR_NS::SdfLayerRefPtr> _subLayersRefs;
    void                                holdOntoSubLayers(SdfLayerHandle layer);
    void                                releaseSubLayers() { _subLayersRefs.clear(); }
    void                                holdOnPathIfDirty(SdfLayerHandle layer, std::string path);
    void                                updateEditTarget(const PXR_NS::UsdStageWeakPtr stage);
};

void BaseCmd::holdOnPathIfDirty(SdfLayerHandle layer, std::string path)
{
    auto subLayerHandle = SdfLayer::FindRelativeToLayer(layer, path);
    if (subLayerHandle != nullptr) {
        if (subLayerHandle->IsDirty() || subLayerHandle->IsAnonymous()) {
            _subLayersRefs.push_back(subLayerHandle);
            holdOntoSubLayers(subLayerHandle); // we'll need to hold onto children as well
        }
    }
}

// hold references to any anon or dirty sublayer
void BaseCmd::holdOntoSubLayers(SdfLayerHandle layer)
{
    const std::vector<std::string>& sublayers = layer->GetSubLayerPaths();
    for (auto path : sublayers) {
        holdOnPathIfDirty(layer, path);
    }
}

// Set the edit target to Session layer if no other layers are modifiable,
// unless the user has disabled this feature with an env var.
void BaseCmd::updateEditTarget(const PXR_NS::UsdStageWeakPtr stage)
{
    //// User-controlled environment variable to disable automatic target change.
    if (TfGetEnvSetting(MAYAUSD_LAYEREDITOR_DISABLE_AUTOTARGET)) {
        return;
    }

    if (!stage)
        return;

    if (stage->GetEditTarget().GetLayer() == stage->GetSessionLayer())
        return;

    // If the currently targeted layer isn't locked, we don't need to change it.
    if (!MayaUsd::isLayerLocked(stage->GetEditTarget().GetLayer()))
        return;

    // If there are no target-able layers, we set the target to session layer.
    std::string errMsg;
    if (!UsdUfe::isAnyLayerModifiable(stage, &errMsg)) {
        MPxCommand::displayInfo(errMsg.c_str());
        stage->SetEditTarget(stage->GetSessionLayer());
    }
}

class InsertRemoveSubPathBase : public BaseCmd
{
public:
    int         _index = -1;
    std::string _subPath;
    std::string _proxyShapePath;

    InsertRemoveSubPathBase(CmdId id)
        : BaseCmd(id)
    {
    }

    bool doIt(SdfLayerHandle layer) override
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
            TF_VERIFY(layer->GetSubLayerPaths()[_index] == _subPath);
        } else {
            TF_VERIFY(_cmdId == CmdId::kRemove);
            if (!validateAndReportIndex(layer, _index, (int)layer->GetNumSubLayerPaths())) {
                return false;
            }
            saveSelection();
            _subPath = layer->GetSubLayerPaths()[_index];
            holdOnPathIfDirty(layer, _subPath);

            // if the current edit target is the layer to remove or
            // a sublayer of the layer to remove,
            // set the root layer as the current edit target
            auto layerToRemove = SdfLayer::FindRelativeToLayer(layer, _subPath);
            auto stage = getStage();
            auto currentTarget = stage->GetEditTarget().GetLayer();

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

                        const auto subLayer
                            = SdfLayer::FindRelativeToLayer(rootLayer, subLayerPath);
                        if (implRef(subLayer, layer, ignore, ignoreSubPath, implRef))
                            return true;
                    }
                    return false;
                };
                return isInHierarchyImpl(
                    rootLayer, layer, ignore, ignoreSubPath, isInHierarchyImpl);
            };

            if (isInHierarchy(layerToRemove, currentTarget)) {
                // The current edit layer is in the hierarchy of the layer to remove,
                // now we need to be sure the edit target layer is not also a sublayer
                // of another layer in the stage.
                if (!isInHierarchy(stage->GetRootLayer(), currentTarget, &layer, &_subPath)) {
                    _editTargetPath = currentTarget->GetIdentifier();
                    stage->SetEditTarget(stage->GetRootLayer());
                }
            }

            layer->RemoveSubLayerPath(_index);
        }
        return true;
    }
    bool undoIt(SdfLayerHandle layer) override
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
                    auto stage = getStage();
                    auto subLayerHandle = SdfLayer::FindRelativeToLayer(layer, _editTargetPath);
                    stage->SetEditTarget(subLayerHandle);
                }
            } else {
                return false;
            }
            restoreSelection();
        }
        return true;
    }
    static bool validateUndoIndex(SdfLayerHandle layer, int index)
    { // allow re-inserting at the last index + 1, but -1 should have been changed to 0
        return !(index < 0 || index > (int)layer->GetNumSubLayerPaths());
    }

    static bool validateAndReportIndex(SdfLayerHandle layer, int index, int maxIndex)
    {
        if (index < 0 || index >= maxIndex) {
            std::string message = std::string("Index ") + std::to_string(index)
                + std::string(" out-of-bound for ") + layer->GetIdentifier();
            MPxCommand::displayError(message.c_str());
            return false;
        } else {
            return true;
        }
    }

    void saveSelection()
    {
        // Make a copy of the global selection, to restore it on undo.
        auto globalSn = Ufe::GlobalSelection::get();
        _savedSn.replaceWith(*globalSn);
        // Filter the global selection, removing items below our proxy shape.
        // We know the path to the proxy shape has a single segment. Not
        // using Ufe::PathString::path() for UFE v1 compatibility, which
        // unfortunately reveals leading "world" path component implementation
        // detail.
        Ufe::Path path(
            Ufe::PathSegment("world" + _proxyShapePath, MayaUsd::ufe::getMayaRunTimeId(), '|'));
        globalSn->replaceWith(UsdUfe::removeDescendants(_savedSn, path));
    }

    void restoreSelection()
    {
        // Restore the saved selection to the global selection.  If a saved
        // selection item started with the proxy shape path, re-create it.
        // We know the path to the proxy shape has a single segment.  Not
        // using Ufe::PathString::path() for UFE v1 compatibility, which
        // unfortunately reveals leading "world" path component implementation
        // detail.
        Ufe::Path path(
            Ufe::PathSegment("world" + _proxyShapePath, MayaUsd::ufe::getMayaRunTimeId(), '|'));
        auto globalSn = Ufe::GlobalSelection::get();
        globalSn->replaceWith(UsdUfe::recreateDescendants(_savedSn, path));
    }

protected:
    std::string    _editTargetPath;
    Ufe::Selection _savedSn;

    UsdStageWeakPtr getStage()
    {
        auto prim = UsdMayaQuery::GetPrim(_proxyShapePath.c_str());
        auto stage = prim.GetStage();
        return stage;
    }
};

class InsertSubPath : public InsertRemoveSubPathBase
{
public:
    InsertSubPath()
        : InsertRemoveSubPathBase(CmdId::kInsert)
    {
    }
};

class RemoveSubPath : public InsertRemoveSubPathBase
{
public:
    RemoveSubPath()
        : InsertRemoveSubPathBase(CmdId::kRemove)
    {
    }
};

// Move a sublayer into another layer.
// @param path The layer's path to move.
// @param newParentLayer The new parent layer's path
// @param newIndex The index where the moved layer will be in the new parent.
class MoveSubPath : public BaseCmd
{
public:
    MoveSubPath(const std::string& path, const std::string& newParentLayer, unsigned newIndex)
        : BaseCmd(CmdId::kMove)
        , _path(path)
        , _newParentLayer(newParentLayer)
        , _newIndex(newIndex)
    {
    }

    bool doIt(SdfLayerHandle layer) override
    {
        auto proxy = layer->GetSubLayerPaths();
        auto subPathIndex = proxy.Find(_path);
        if (subPathIndex == size_t(-1)) {
            std::string message = std::string("path ") + _path + std::string(" not found on layer ")
                + layer->GetIdentifier();
            MPxCommand::displayError(message.c_str());
            return false;
        }

        _oldIndex = subPathIndex; // save for undo

        SdfLayerHandle newParentLayer;
        std::string    newPath = _path;

        if (layer->GetIdentifier() == _newParentLayer) {

            if (_newIndex > layer->GetNumSubLayerPaths() - 1) {
                std::string message = std::string("Index ") + std::to_string(_newIndex)
                    + std::string(" out-of-bound for ") + layer->GetIdentifier();
                MPxCommand::displayError(message.c_str());
                return false;
            }

            newParentLayer = layer;
        } else {
            newParentLayer = SdfLayer::Find(_newParentLayer);
            if (!newParentLayer) {
                std::string message
                    = std::string("Layer ") + _newParentLayer + std::string(" not found!");
                return false;
            }

            if (_newIndex > newParentLayer->GetNumSubLayerPaths()) {
                std::string message = std::string("Index ") + std::to_string(_newIndex)
                    + std::string(" out-of-bound for ") + newParentLayer->GetIdentifier();
                MPxCommand::displayError(message.c_str());
                return false;
            }

            // See if the path should be reparented
            ghc::filesystem::path filePath(_path);
            bool                  needsRepathing = !SdfLayer::IsAnonymousLayerIdentifier(_path);
            needsRepathing &= filePath.is_relative();
            needsRepathing &= !layer->GetRealPath().empty();
            needsRepathing &= !newParentLayer->GetRealPath().empty();

            // Reparent the path if needed
            if (needsRepathing) {
                auto oldLayerDir = ghc::filesystem::path(layer->GetRealPath()).remove_filename();
                auto newLayerDir
                    = ghc::filesystem::path(newParentLayer->GetRealPath()).remove_filename();

                std::string absolutePath
                    = (oldLayerDir / filePath).lexically_normal().generic_string();
                auto result = UsdMayaUtilFileSystem::makePathRelativeTo(
                    absolutePath, newLayerDir.lexically_normal().generic_string());

                if (result.second) {
                    newPath = result.first;
                } else {
                    newPath = absolutePath;
                    TF_WARN(
                        "File name (%s) cannot be resolved as relative to the layer %s, "
                        "using the absolute path.",
                        absolutePath.c_str(),
                        newParentLayer->GetIdentifier().c_str());
                }
            }

            // make sure the subpath is not already in the new parent layer.
            // Otherwise, the SdfLayer::InsertSubLayerPath() below will do nothing
            // and the subpath will be removed from it's current parent.
            if (newParentLayer->GetSubLayerPaths().Find(newPath) != size_t(-1)) {
                std::string message = std::string("SubPath ") + newPath
                    + std::string(" already exist in layer ") + newParentLayer->GetIdentifier();
                MPxCommand::displayError(message.c_str());
                return false;
            }
        }

        // When the subLayer is moved inside the current parent,
        // Remove it from it's current location and insert it into it's
        // new location. The order of remove / insert is important
        // oterwise InsertSubLayerPath() will fail because the subLayer
        // already exists.
        layer->RemoveSubLayerPath(subPathIndex);
        newParentLayer->InsertSubLayerPath(newPath, _newIndex);

        return true;
    }

    bool undoIt(SdfLayerHandle layer) override
    {
        if (layer->GetIdentifier() == _newParentLayer) {
            // When the subLayer is moved inside the current parent,
            // Remove it from it's current location and insert it into it's
            // new location. The order of remove / insert is important
            // oterwise InsertSubLayerPath() will fail because the subLayer
            // already exists.
            layer->RemoveSubLayerPath(_newIndex);
            layer->InsertSubLayerPath(_path, _oldIndex);
        } else {
            auto newParentLayer = SdfLayer::Find(_newParentLayer);
            newParentLayer->RemoveSubLayerPath(_newIndex);
            layer->InsertSubLayerPath(_path, _oldIndex);
        }

        return true;
    }

private:
    std::string  _path;
    std::string  _newParentLayer;
    unsigned int _newIndex;
    unsigned int _oldIndex { 0 };
};

class ReplaceSubPath : public BaseCmd
{
public:
    ReplaceSubPath()
        : BaseCmd(CmdId::kReplace)
    {
    }

    bool doIt(SdfLayerHandle layer) override
    {
        auto proxy = layer->GetSubLayerPaths();
        if (proxy.Find(_oldPath) == size_t(-1)) {
            std::string message = std::string("path ") + _oldPath
                + std::string(" not found on layer ") + layer->GetIdentifier();
            MPxCommand::displayError(message.c_str());
            return false;
        }

        holdOnPathIfDirty(layer, _oldPath);
        proxy.Replace(_oldPath, _newPath);
        return true;
    }

    bool undoIt(SdfLayerHandle layer) override
    {
        auto proxy = layer->GetSubLayerPaths();
        proxy.Replace(_newPath, _oldPath);
        releaseSubLayers();
        holdOnPathIfDirty(layer, _newPath);
        return true;
    }

    std::string _oldPath, _newPath;
};

class AddAnonSubLayer : public InsertRemoveSubPathBase
{
public:
    AddAnonSubLayer()
        : InsertRemoveSubPathBase(CmdId::kAddAnonLayer) {};

    bool doIt(SdfLayerHandle layer) override
    {
        // the first time, USD will create a layer with a certain identifier
        // on undo(), we will remove the path, but hold onto the layer
        // on redo, we want to put back that same identifier, for later commands
        if (_anonIdentifier.empty()) {
            _anonLayer = SdfLayer::CreateAnonymous(_anonName);
            _anonIdentifier = _anonLayer->GetIdentifier();
        }
        _subPath = _anonIdentifier;
        _index = 0;
        _cmdResult = _subPath;
        return InsertRemoveSubPathBase::doIt(layer);
    }

    bool undoIt(SdfLayerHandle layer) override { return InsertRemoveSubPathBase::undoIt(layer); }

    std::string _anonName;

protected:
    PXR_NS::SdfLayerRefPtr _anonLayer;
    std::string            _anonIdentifier;
};

class BackupLayerBase : public BaseCmd
{
    // commands that need to backup the whole layer for undo
public:
    BackupLayerBase(CmdId id)
        : BaseCmd(id)
    {
    }

    bool doIt(SdfLayerHandle layer) override
    {
        backupLayer(layer);

        // using reload will correctly reset the dirty bit
        holdOntoSubLayers(layer);

        if (_cmdId == CmdId::kDiscardEdit) {
            layer->Reload();
        } else if (_cmdId == CmdId::kClearLayer) {
            layer->Clear();
        }

        // Note: backup the edit targets after the layer is cleared because we use
        //       the fact that a stage edit target is now invalid to decide to backup
        //       that edit target.
        backupEditTargets(layer);

        return true;
    }

    bool undoIt(SdfLayerHandle layer) override
    {
        restoreLayer(layer);

        // Note: restore edit targets after the layers are restored so that the backup
        //       edit targets are now valid.
        restoreEditTargets();

        releaseSubLayers();

        return true;
    }

private:
    // Backup and restore dirty layers to support undo and redo.
    void backupLayer(SdfLayerHandle layer)
    {
        if (!layer)
            return;

        if (layer->IsDirty() || _cmdId == CmdId::kClearLayer) {
            _backupLayer = SdfLayer::CreateAnonymous();
            _backupLayer->TransferContent(layer);
        }
    }

    void restoreLayer(SdfLayerHandle layer)
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

    // Backup and restore edit targets of stages that were targeting the sub-layers
    // of the cleared layer to support undo and redo.
    void backupEditTargets(SdfLayerHandle layer)
    {
        _editTargetBackups.clear();

        if (!layer)
            return;

        const UsdMayaStageCache::Caches& caches = UsdMayaStageCache::GetAllCaches();
        for (const PXR_NS::UsdStageCache& cache : caches) {
            const std::vector<UsdStageRefPtr> stages = cache.GetAllStages();
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
    }

    void restoreEditTargets()
    {
        for (const auto& weakStageAndTarget : _editTargetBackups) {
            const PXR_NS::UsdStageRefPtr stage = weakStageAndTarget.first;
            if (!stage)
                continue;

            PXR_NS::UsdEditTarget target = weakStageAndTarget.second;
            stage->SetEditTarget(target);
        }
    }

    // Edit targets that were made invalid after the layer was cleared.
    // The stages are kept with weak pointer to avoid forcing to stay valid.
    using EditTargetBackups = std::map<PXR_NS::UsdStagePtr, PXR_NS::UsdEditTarget>;
    EditTargetBackups _editTargetBackups;

    // we need to hold onto the layer if we dirty it
    PXR_NS::SdfLayerRefPtr _backupLayer;
};

class DiscardEdit : public BackupLayerBase
{
public:
    DiscardEdit()
        : BackupLayerBase(CmdId::kDiscardEdit)
    {
    }
};

class ClearLayer : public BackupLayerBase
{
public:
    ClearLayer()
        : BackupLayerBase(CmdId::kClearLayer)
    {
    }
};

class MuteLayer : public BaseCmd
{
public:
    MuteLayer()
        : BaseCmd(CmdId::kMuteLayer)
    {
    }

    bool doIt(SdfLayerHandle layer) override
    {
        auto stage = getStage();
        if (!stage)
            return false;
        if (_muteIt) {
            // Muting a layer will cause all scene items under the proxy shape
            // to be stale.
            saveSelection();
            stage->MuteLayer(layer->GetIdentifier());
        } else {
            stage->UnmuteLayer(layer->GetIdentifier());
            restoreSelection();
        }

        // We prefer not holding to pointers needlessly, but we need to hold on
        // to the muted layer. OpenUSD let go of muted layers, so anonymous
        // layers and any dirty children would be lost if not explicitly held on.
        addMutedLayer(layer);

        updateEditTarget(stage);

        return true;
    }

    bool undoIt(SdfLayerHandle layer) override
    {
        auto stage = getStage();
        if (!stage)
            return false;
        if (_muteIt) {
            stage->UnmuteLayer(layer->GetIdentifier());
            restoreSelection();
        } else {
            // Muting a layer will cause all scene items under the proxy shape
            // to be stale.
            saveSelection();
            stage->MuteLayer(layer->GetIdentifier());
        }

        // We can release the now unmuted layers.
        removeMutedLayer(layer);

        updateEditTarget(stage);

        return true;
    }

    std::string _proxyShapePath;
    bool        _muteIt = true;

private:
    UsdStageWeakPtr getStage()
    {
        auto prim = UsdMayaQuery::GetPrim(_proxyShapePath.c_str());
        auto stage = prim.GetStage();
        return stage;
    }

    void saveSelection()
    {
        // Make a copy of the global selection, to restore it on unmute.
        auto globalSn = Ufe::GlobalSelection::get();
        _savedSn.replaceWith(*globalSn);
        // Filter the global selection, removing items below our proxy shape.
        // We know the path to the proxy shape has a single segment.  Not
        // using Ufe::PathString::path() for UFE v1 compatibility, which
        // unfortunately reveals leading "world" path component implementation
        // detail.
        Ufe::Path path(
            Ufe::PathSegment("world" + _proxyShapePath, MayaUsd::ufe::getMayaRunTimeId(), '|'));
        globalSn->replaceWith(UsdUfe::removeDescendants(_savedSn, path));
    }

    void restoreSelection()
    {
        // Restore the saved selection to the global selection.  If a saved
        // selection item started with the proxy shape path, re-create it.
        // We know the path to the proxy shape has a single segment.  Not
        // using Ufe::PathString::path() for UFE v1 compatibility, which
        // unfortunately reveals leading "world" path component implementation
        // detail.
        Ufe::Path path(
            Ufe::PathSegment("world" + _proxyShapePath, MayaUsd::ufe::getMayaRunTimeId(), '|'));
        auto globalSn = Ufe::GlobalSelection::get();
        globalSn->replaceWith(UsdUfe::recreateDescendants(_savedSn, path));
    }

    Ufe::Selection _savedSn;
};

class LockLayer : public BaseCmd
{
public:
    LockLayer()
        : BaseCmd(CmdId::kLockLayer)
    {
    }

    bool doIt(SdfLayerHandle layer) override
    {
        auto stage = getStage();
        if (!stage)
            return false;

        std::set<PXR_NS::SdfLayerRefPtr> layersToUpdate;
        if (_includeSublayers) {
            // If _includeSublayers is True, we attempt to refresh the system lock status of all
            // layers under the given layer. This is specially useful when reloading a stage.
            bool includeTopLayer = true;
            layersToUpdate = MayaUsd::getAllSublayerRefs(layer, includeTopLayer);
        } else {
            layersToUpdate.insert(layer);
        }

        for (auto layerIt : layersToUpdate) {
            if (MayaUsd::isLayerLocked(layerIt)) {
                _previousStates.push_back(MayaUsd::LayerLockType::LayerLock_Locked);
            } else if (MayaUsd::isLayerSystemLocked(layerIt)) {
                _previousStates.push_back(MayaUsd::LayerLockType::LayerLock_SystemLocked);
            } else {
                _previousStates.push_back(MayaUsd::LayerLockType::LayerLock_Unlocked);
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
                    if (_lockType != MayaUsd::LayerLockType::LayerLock_SystemLocked) {
                        if (MayaUsd::isLayerSystemLocked(curLayer)) {
                            continue;
                        }
                    }
                }
            }

            MayaUsd::lockLayer(_proxyShapePath, curLayer, _lockType, true);
        }

        if (_updateEditTarget) {
            updateEditTarget(stage);
        }

        return true;
    }

    bool undoIt(SdfLayerHandle layer) override
    {
        auto stage = getStage();
        if (!stage)
            return false;

        if (_layers.size() != _previousStates.size()) {
            return false;
        }
        // Execute lock commands
        for (size_t layerIndex = 0; layerIndex < _layers.size(); layerIndex++) {
            // Note: the undo of system-locked is unlocked by design.
            if (_lockType == MayaUsd::LayerLockType::LayerLock_SystemLocked) {
                MayaUsd::lockLayer(
                    _proxyShapePath,
                    _layers[layerIndex],
                    MayaUsd::LayerLockType::LayerLock_Unlocked,
                    true);
            } else {
                MayaUsd::lockLayer(
                    _proxyShapePath, _layers[layerIndex], _previousStates[layerIndex], true);
            }
        }

        if (_updateEditTarget) {
            updateEditTarget(stage);
        }

        return true;
    }

    MayaUsd::LayerLockType _lockType = MayaUsd::LayerLockType::LayerLock_Locked;
    bool                   _includeSublayers = false;
    bool                   _skipSystemLockedLayers = false;
    bool                   _updateEditTarget = true;
    std::string            _proxyShapePath;

private:
    UsdStageWeakPtr getStage()
    {
        auto prim = UsdMayaQuery::GetPrim(_proxyShapePath.c_str());
        auto stage = prim.GetStage();
        return stage;
    }

    std::vector<MayaUsd::LayerLockType> _previousStates;
    SdfLayerHandleVector                _layers;
};

class RefreshSystemLockLayer : public BaseCmd
{
public:
    RefreshSystemLockLayer()
        : BaseCmd(CmdId::kRefreshSystemLock)
    {
    }

    bool doIt(SdfLayerHandle layer) override
    {
        auto stage = getStage();
        if (!stage)
            return false;

        if (_refreshSubLayers) {
            // If refreshSubLayers is True, we attempt to refresh the system lock status of all
            // layers under the given layer. This is specially useful when reloading a stage.
            bool includeTopLayer = true;
            auto allLayers = MayaUsd::getAllSublayerRefs(layer, includeTopLayer);
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
            updateEditTarget(stage);
        }

        return true;
    }

    // The command itself doesn't retain its state. However, the underlying logic contains commands
    // that are undoable.
    bool undoIt(SdfLayerHandle layer) override
    {
        auto stage = getStage();
        if (!stage)
            return false;

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
            updateEditTarget(stage);
        }

        return true;
    }

    std::string                                 _proxyShapePath;
    bool                                        _refreshSubLayers = false;
    std::vector<std::shared_ptr<Impl::BaseCmd>> _lockCommands;
    SdfLayerHandleVector                        _layers;

private:
    std::string _quote(const std::string& string)
    {
        return std::string(" \"") + string + std::string("\"");
    }

    // Checks if the file layer or its sublayers are accessible on disk, and adds the layer
    // to _layers along with the _lockCommands to updates the system-lock status.
    void _refreshLayerSystemLock(SdfLayerHandle usdLayer)
    {
        // Anonymous layers do not need to be checked.
        if (usdLayer && !usdLayer->IsAnonymous()) {
            // Check if the layer's write permissions have changed.
            std::string assetPath = usdLayer->GetResolvedPath();
            std::replace(assetPath.begin(), assetPath.end(), '\\', '/');

            if (!assetPath.empty()) {
                MString commandString;
                commandString.format("filetest -w \"^1s\"", MString(assetPath.c_str()));
                MIntArray result;
                // filetest is NOT undoable
                MGlobal::executeCommand(commandString, result, /*display*/ false, /*undo*/ false);
                if (result.length() > 0) {

                    if (result[0] == 1 && MayaUsd::isLayerSystemLocked(usdLayer)) {
                        // If the file has write permissions and the layer is currently
                        // system-locked: Unlock the layer

                        // Create the lock command
                        auto cmd = std::make_shared<Impl::LockLayer>();
                        cmd->_lockType = MayaUsd::LayerLockType::LayerLock_Unlocked;
                        cmd->_includeSublayers = false;
                        cmd->_proxyShapePath = _proxyShapePath;
                        // Edit target will be updated once at the end of the refresh command.
                        cmd->_updateEditTarget = false;

                        // Add the lock command and its parameter to be executed
                        _lockCommands.push_back(std::move(cmd));
                        _layers.push_back(usdLayer);
                    } else if (result[0] == 0 && !MayaUsd::isLayerSystemLocked(usdLayer)) {
                        // If the file doesn't have write permissions and the layer is currently not
                        // system-locked: System-lock the layer

                        // Create the lock command
                        auto cmd = std::make_shared<Impl::LockLayer>();
                        cmd->_lockType = MayaUsd::LayerLockType::LayerLock_SystemLocked;
                        cmd->_includeSublayers = false;
                        cmd->_proxyShapePath = _proxyShapePath;
                        // Edit target will be updated once at the end of the refresh command.
                        cmd->_updateEditTarget = false;

                        // Add the lock command and its parameter to be executed
                        _lockCommands.push_back(std::move(cmd));
                        _layers.push_back(usdLayer);
                    }
                }
            }
        }
    }

    void _notifySystemLockIsRefreshed()
    {
        if (!UsdUfe::isUICallbackRegistered(TfToken("onRefreshSystemLock")))
            return;

        PXR_NS::VtDictionary callbackContext;
        callbackContext["proxyShapePath"] = PXR_NS::VtValue(_proxyShapePath.c_str());
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

    UsdStageWeakPtr getStage()
    {
        auto prim = UsdMayaQuery::GetPrim(_proxyShapePath.c_str());
        auto stage = prim.GetStage();
        return stage;
    }
};

// We assume the indexes given to the command are the original indexes
// of the layers. Since each command is executed individually and in
// order, each one may affect the index of subsequent commands. We
// records adjustements that must be applied to indexes in the map.
// Removal of a layer creates a negative adjustment, insertion of a
// layer creates a positive adjustment.
class IndexAdjustments
{
public:
    IndexAdjustments() = default;

    // Convenience method that retrieve the adjusted index and adds
    // the insertion index adjustment.
    int insertionAdjustment(int originalIndex)
    {
        const int adjustedIndex = getAdjustedIndex(originalIndex);
        addInsertionAdjustment(originalIndex);
        return adjustedIndex;
    }

    // Convenience method that retrieve the adjusted index and adds
    // the removal index adjustment.
    int removalAdjustment(int originalIndex)
    {
        const int adjustedIndex = getAdjustedIndex(originalIndex);
        addRemovalAdjustment(originalIndex);
        return adjustedIndex;
    }

private:
    // Insertion and removal additional adjustment.
    // Must be called with the original index as provided by the user.
    void addInsertionAdjustment(int index) { _indexAdjustments[index] += 1; }
    void addRemovalAdjustment(int index) { _indexAdjustments[index] -= 1; }

    // Calculate the adjusted index from the user-supplied index that
    // need to be used by the command to account for previous commands.
    int getAdjustedIndex(int index) const
    {
        // Apply all adjustment that were done on indexes lower or
        // equal to the input index.
        int adjustedIndex = index;
        for (const auto& indexAndAdjustement : _indexAdjustments) {
            if (indexAndAdjustement.first > index)
                break;
            adjustedIndex += indexAndAdjustement.second;
        }
        return adjustedIndex;
    }

    std::map<int, int> _indexAdjustments;
};

} // namespace Impl

const char LayerEditorCommand::commandName[] = "mayaUsdLayerEditor";

// plug-in callback to create the command object
void* LayerEditorCommand::creator() { return static_cast<MPxCommand*>(new LayerEditorCommand()); }

// plug-in callback to register the command syntax
MSyntax LayerEditorCommand::createSyntax()
{
    MSyntax syntax;

    // syntax.enableQuery(true);
    syntax.enableEdit(true);

    // layer id
    syntax.setObjectType(MSyntax::kStringObjects, 1, 1);

    syntax.addFlag(kInsertSubPathFlag, kInsertSubPathFlagL, MSyntax::kLong, MSyntax::kString);
    syntax.makeFlagMultiUse(kInsertSubPathFlag);
    syntax.addFlag(kRemoveSubPathFlag, kRemoveSubPathFlagL, MSyntax::kLong, MSyntax::kString);
    syntax.makeFlagMultiUse(kRemoveSubPathFlag);
    syntax.addFlag(kReplaceSubPathFlag, kReplaceSubPathFlagL, MSyntax::kString, MSyntax::kString);
    syntax.makeFlagMultiUse(kReplaceSubPathFlag);
    syntax.addFlag(
        kMoveSubPathFlag,
        kMoveSubPathFlagL,
        MSyntax::kString,    // path to move
        MSyntax::kString,    // new parent layer
        MSyntax::kUnsigned); // layer index inside the new parent
    syntax.addFlag(kDiscardEditsFlag, kDiscardEditsFlagL);
    syntax.addFlag(kClearLayerFlag, kClearLayerFlagL);
    // parameter: new layer name
    syntax.addFlag(kAddAnonSublayerFlag, kAddAnonSublayerFlagL, MSyntax::kString);
    syntax.makeFlagMultiUse(kAddAnonSublayerFlag);
    // parameter: proxy shape name
    syntax.addFlag(kMuteLayerFlag, kMuteLayerFlagL, MSyntax::kBoolean, MSyntax::kString);
    syntax.addFlag(
        kLockLayerFlag, kLockLayerFlagL, MSyntax::kLong, MSyntax::kBoolean, MSyntax::kString);
    // parameter 1: proxy shape name
    // parameter 2: refresh sub layers
    syntax.addFlag(
        kRefreshSystemLockFlag, kRefreshSystemLockFlagL, MSyntax::kString, MSyntax::kBoolean);
    syntax.addFlag(kSkipSystemLockedFlag, kSkipSystemLockedFlagL);

    return syntax;
}

// private argument parsing helper
MStatus LayerEditorCommand::parseArgs(const MArgList& argList)
{
    setCommandString(commandName);

    MStatus    status;
    MArgParser argParser(syntax(), argList, &status);
    if (status != MS::kSuccess) {
        return MS::kInvalidParameter;
    }
    if (argParser.isQuery()) {
        _cmdMode = Mode::Query;
    } else if (argParser.isEdit()) {
        _cmdMode = Mode::Edit;
    } else {
        _cmdMode = Mode::Create;
    }

    MStringArray objects;
    argParser.getObjects(objects);
    _layerIdentifier = objects[0].asChar();

    if (!isQuery()) {
        Impl::IndexAdjustments indexAdjustments;

        const bool skipSystemLockedLayers = argParser.isFlagSet(kSkipSystemLockedFlag);

        if (argParser.isFlagSet(kInsertSubPathFlag)) {
            auto count = argParser.numberOfFlagUses(kInsertSubPathFlag);
            for (unsigned i = 0; i < count; i++) {
                auto cmd = std::make_shared<Impl::InsertSubPath>();

                MArgList listOfArgs;
                argParser.getFlagArgumentList(kInsertSubPathFlag, i, listOfArgs);

                const int originalIndex = listOfArgs.asInt(0);
                const int adjustedIndex = indexAdjustments.insertionAdjustment(originalIndex);

                cmd->_index = adjustedIndex;
                cmd->_subPath = listOfArgs.asString(1).asUTF8();

                _subCommands.push_back(std::move(cmd));
            }
        }

        if (argParser.isFlagSet(kRemoveSubPathFlag)) {
            auto count = argParser.numberOfFlagUses(kRemoveSubPathFlag);
            for (unsigned i = 0; i < count; i++) {

                MArgList listOfArgs;
                argParser.getFlagArgumentList(kRemoveSubPathFlag, i, listOfArgs);

                auto shapePath = listOfArgs.asString(1);
                auto prim = UsdMayaQuery::GetPrim(shapePath.asChar());
                if (prim == UsdPrim()) {
                    displayError(MString("Invalid proxy shape \"") + shapePath.asChar() + "\"");
                    return MS::kInvalidParameter;
                }

                const int originalIndex = listOfArgs.asInt(0);
                const int adjustedIndex = indexAdjustments.removalAdjustment(originalIndex);

                auto cmd = std::make_shared<Impl::RemoveSubPath>();
                cmd->_index = adjustedIndex;
                cmd->_proxyShapePath = shapePath.asChar();
                _subCommands.push_back(std::move(cmd));
            }
        }

        if (argParser.isFlagSet(kReplaceSubPathFlag)) {
            auto count = argParser.numberOfFlagUses(kReplaceSubPathFlag);
            for (unsigned i = 0; i < count; i++) {
                auto cmd = std::make_shared<Impl::ReplaceSubPath>();

                MArgList listOfArgs;
                argParser.getFlagArgumentList(kReplaceSubPathFlag, i, listOfArgs);
                cmd->_oldPath = listOfArgs.asString(0).asUTF8();
                cmd->_newPath = listOfArgs.asString(1).asUTF8();
                _subCommands.push_back(std::move(cmd));
            }
        }

        if (argParser.isFlagSet(kMoveSubPathFlag)) {
            MString subPath;
            argParser.getFlagArgument(kMoveSubPathFlag, 0, subPath);

            MString newParentLayer;
            argParser.getFlagArgument(kMoveSubPathFlag, 1, newParentLayer);

            int originalIndex { 0 };
            argParser.getFlagArgument(kMoveSubPathFlag, 2, originalIndex);
            const int adjustedIndex = indexAdjustments.removalAdjustment(originalIndex);

            auto cmd = std::make_shared<Impl::MoveSubPath>(
                subPath.asUTF8(), newParentLayer.asUTF8(), adjustedIndex);
            _subCommands.push_back(std::move(cmd));
        }

        if (argParser.isFlagSet(kDiscardEditsFlag)) {
            auto cmd = std::make_shared<Impl::DiscardEdit>();
            _subCommands.push_back(std::move(cmd));
        }

        if (argParser.isFlagSet(kClearLayerFlag)) {
            auto cmd = std::make_shared<Impl::ClearLayer>();
            _subCommands.push_back(std::move(cmd));
        }

        if (argParser.isFlagSet(kAddAnonSublayerFlag)) {
            auto count = argParser.numberOfFlagUses(kAddAnonSublayerFlag);
            for (unsigned i = 0; i < count; i++) {
                auto cmd = std::make_shared<Impl::AddAnonSubLayer>();

                MArgList listOfArgs;
                argParser.getFlagArgumentList(kAddAnonSublayerFlag, i, listOfArgs);
                cmd->_anonName = listOfArgs.asString(0).asUTF8();
                _subCommands.push_back(std::move(cmd));
            }
        }
        if (argParser.isFlagSet(kMuteLayerFlag)) {
            bool muteIt = true;
            argParser.getFlagArgument(kMuteLayerFlag, 0, muteIt);

            MString proxyShapeName;
            argParser.getFlagArgument(kMuteLayerFlag, 1, proxyShapeName);

            auto prim = UsdMayaQuery::GetPrim(proxyShapeName.asChar());
            if (prim == UsdPrim()) {
                displayError(
                    MString("Invalid proxy shape \"") + MString(proxyShapeName.asChar()) + "\"");
                return MS::kInvalidParameter;
            }

            auto cmd = std::make_shared<Impl::MuteLayer>();
            cmd->_muteIt = muteIt;
            cmd->_proxyShapePath = proxyShapeName.asChar();
            _subCommands.push_back(std::move(cmd));
        }
        if (argParser.isFlagSet(kLockLayerFlag)) {
            int lockValue = 0;
            // 0 = Unlocked
            // 1 = Locked
            // 2 = SystemLocked
            argParser.getFlagArgument(kLockLayerFlag, 0, lockValue);

            bool includeSublayers = false;
            argParser.getFlagArgument(kLockLayerFlag, 1, includeSublayers);

            MString proxyShapeName;
            argParser.getFlagArgument(kLockLayerFlag, 2, proxyShapeName);

            auto prim = UsdMayaQuery::GetPrim(proxyShapeName.asChar());
            if (prim == UsdPrim()) {
                displayError(
                    MString("Invalid proxy shape \"") + MString(proxyShapeName.asChar()) + "\"");
                return MS::kInvalidParameter;
            }

            auto cmd = std::make_shared<Impl::LockLayer>();
            switch (lockValue) {
            case 1: {
                cmd->_lockType = MayaUsd::LayerLockType::LayerLock_Locked;
                break;
            }
            case 2: {
                cmd->_lockType = MayaUsd::LayerLockType::LayerLock_SystemLocked;
                break;
            }
            default: {
                cmd->_lockType = MayaUsd::LayerLockType::LayerLock_Unlocked;
                break;
            }
            }
            cmd->_includeSublayers = includeSublayers;
            cmd->_skipSystemLockedLayers = skipSystemLockedLayers;
            cmd->_proxyShapePath = proxyShapeName.asChar();
            _subCommands.push_back(std::move(cmd));
        }
        if (argParser.isFlagSet(kRefreshSystemLockFlag)) {
            MString proxyShapeName;
            argParser.getFlagArgument(kRefreshSystemLockFlag, 0, proxyShapeName);
            bool refreshSubLayers = true;
            argParser.getFlagArgument(kRefreshSystemLockFlag, 1, refreshSubLayers);

            auto prim = UsdMayaQuery::GetPrim(proxyShapeName.asChar());
            if (prim == UsdPrim()) {
                displayError(
                    MString("Invalid proxy shape \"") + MString(proxyShapeName.asChar()) + "\"");
                return MS::kInvalidParameter;
            }

            auto cmd = std::make_shared<Impl::RefreshSystemLockLayer>();
            cmd->_proxyShapePath = proxyShapeName.asChar();
            cmd->_refreshSubLayers = refreshSubLayers;
            _subCommands.push_back(std::move(cmd));
        }
    }

    return MS::kSuccess;
}

// MPxCommand undo ability callback
bool LayerEditorCommand::isUndoable() const { return !isQuery(); }

// main MPxCommand execution point
MStatus LayerEditorCommand::doIt(const MArgList& argList)
{
    MStatus status(MS::kSuccess);
    clearResult();

    status = parseArgs(argList);
    if (status != MS::kSuccess)
        return status;

    return redoIt();
}

// main MPxCommand execution point
MStatus LayerEditorCommand::redoIt()
{

    auto layer = SdfLayer::FindOrOpen(_layerIdentifier);
    if (!layer) {
        return MS::kInvalidParameter;
    }

    for (auto it = _subCommands.begin(); it != _subCommands.end(); ++it) {
        if (!(*it)->doIt(layer)) {
            return MS::kFailure;
        }
        const auto& result = (*it)->_cmdResult;
        if (!result.empty()) {
            appendToResult(result.c_str());
        }
    }

    return MS::kSuccess;
}

// main MPxCommand execution point
MStatus LayerEditorCommand::undoIt()
{

    auto layer = SdfLayer::FindOrOpen(_layerIdentifier);
    if (!layer) {
        return MS::kInvalidParameter;
    }

    // clang-format off
    for (auto it = _subCommands.rbegin(); it != _subCommands.rend(); ++it) { 
        if (!(*it)->undoIt(layer)) {
            return MS::kFailure;
        }
    }

    // clang-format on
    return MS::kSuccess;
}

} // namespace MAYAUSD_NS_DEF
