//
// Copyright 2024 Autodesk
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
#ifndef LAYER_EDITOR_COMMANDS_H
#define LAYER_EDITOR_COMMANDS_H

#include <usdLayerEditor/lib/abstractCommandHook.h>
#include <usdLayerEditor/lib/layerEditorAPI.h>

#include <usdUfe/undo/UsdUndoableItem.h>

#include <pxr/base/vt/dictionary.h>
#include <pxr/base/vt/value.h>

#include <ufe/selection.h>
#include <ufe/undoableCommand.h>

#include <functional>

namespace UsdLayerEditor {

enum class CmdId
{
    kInsert,
    kRemove,
    kMove,
    kReplace,
    kDiscardEdit,
    kClearLayer,
    kFlattenLayer,
    kAddAnonLayer,
    kMuteLayer,
    kLockLayer,
    kRefreshSystemLock,
    kStitchLayers
};

class LAYEREDITOR_PUBLIC BaseCmd : public Ufe::UndoableCommand
{
public:
    BaseCmd(CmdId id, const PXR_NS::SdfLayerRefPtr& layer)
        : _cmdId(id)
        , _layer(layer)
    {
    }
    virtual ~BaseCmd() { }

    void execute() override { redo(); }
    void undo() override;
    void redo() override;

    virtual bool doIt(const PXR_NS::SdfLayerHandle& layer) = 0;
    virtual bool undoIt(const PXR_NS::SdfLayerHandle& layer) = 0;

protected:
    CmdId               _cmdId;
    std::string         _cmdResult; // set if the command returns something
    PXR_NS::SdfLayerRefPtr _layer;

    // we need to hold on to dirty sublayers if we remove them
    std::vector<PXR_NS::SdfLayerRefPtr> _subLayersRefs;
    void                                holdOntoSubLayers(const PXR_NS::SdfLayerHandle& layer);
    void                                releaseSubLayers() { _subLayersRefs.clear(); }
    void holdOnPathIfDirty(const PXR_NS::SdfLayerHandle& layer, const std::string& path);
    void updateEditTarget(const PXR_NS::UsdStageWeakPtr stage);
};

class LAYEREDITOR_PUBLIC BackupLayerBaseCmd : public BaseCmd
{
    // commands that need to backup the whole layer for undo
public:
    BackupLayerBaseCmd(CmdId id, const PXR_NS::SdfLayerRefPtr& layer)
        : BaseCmd(id, layer)
    {
    }

    bool doIt(const PXR_NS::SdfLayerHandle& layer) override;

    bool undoIt(const PXR_NS::SdfLayerHandle& layer) override;

protected:
    // Backup and restore edit targets of stages that were targeting the sub-layers
    // of the cleared layer to support undo and redo.
    void backupEditTargets(const PXR_NS::SdfLayerHandle& layer);

    void restoreEditTargets();

private:
    // Backup dirty layer to support undo and redo.
    void backupLayer(const PXR_NS::SdfLayerHandle& layer);

    void restoreLayer(const PXR_NS::SdfLayerHandle& layer);

    // Edit targets that were made invalid after the layer was cleared.
    // The stages are kept with weak pointer to avoid forcing to stay valid.
    using EditTargetBackups = std::map<PXR_NS::UsdStagePtr, PXR_NS::UsdEditTarget>;
    EditTargetBackups _editTargetBackups;

    // we need to hold onto the layer if we dirty it
    PXR_NS::SdfLayerRefPtr _backupLayer;
};

class LAYEREDITOR_PUBLIC ClearLayerCmd : public BackupLayerBaseCmd
{
public:
    ClearLayerCmd(const PXR_NS::SdfLayerRefPtr& layer)
        : BackupLayerBaseCmd(CmdId::kClearLayer, layer)
    {
    }
    std::string commandString() const override { return "Clear USD layer"; }
};

class LAYEREDITOR_PUBLIC FlattenLayerCmd : public BackupLayerBaseCmd
{
public:
    FlattenLayerCmd(const PXR_NS::SdfLayerRefPtr& layer)
        : BackupLayerBaseCmd(CmdId::kFlattenLayer, layer)
    {
    }
    std::string commandString() const override { return "Merge with Sublayers"; }
};

class LAYEREDITOR_PUBLIC DiscardEditCmd : public BackupLayerBaseCmd
{
public:
    DiscardEditCmd(const PXR_NS::SdfLayerRefPtr& layer)
        : BackupLayerBaseCmd(CmdId::kDiscardEdit, layer)
    {
    }
    std::string commandString() const override { return "Reload USD layer"; }
};

class LAYEREDITOR_PUBLIC SetEditTargetCmd : public Ufe::UndoableCommand
{

public:
    SetEditTargetCmd(const PXR_NS::UsdStagePtr& stage, UsdLayerEditor::UsdLayer usdLayer)
        : stage(stage)
        , newTarget(usdLayer)
        , oldTarget(stage->GetEditTarget())
    {
    }

    void        execute() override { stage->SetEditTarget(newTarget); }
    void        undo() override { stage->SetEditTarget(oldTarget); }
    void        redo() override { stage->SetEditTarget(newTarget); }
    std::string commandString() const override { return "Set Edit Target"; }

private:
    const PXR_NS::UsdStagePtr   stage;
    UsdLayerEditor::UsdLayer newTarget;
    PXR_NS::UsdEditTarget       oldTarget;
};

class LAYEREDITOR_PUBLIC MuteLayerCmd : public BaseCmd
{
public:
    MuteLayerCmd(const PXR_NS::UsdStageRefPtr& stage, const PXR_NS::SdfLayerRefPtr& layer, bool muteIt)
        : BaseCmd(CmdId::kMuteLayer, layer)
        , _stage(stage)
        , _muteIt(muteIt)
    {
    }

    bool doIt(const PXR_NS::SdfLayerHandle& layer) override;

    bool undoIt(const PXR_NS::SdfLayerHandle& layer) override;

    std::string commandString() const override { return _muteIt ? "Mute" : "Unmute"; }

private:
    PXR_NS::UsdStageWeakPtr getStage();

    void saveSelection();

    void restoreSelection();

    const PXR_NS::UsdStageRefPtr _stage = nullptr;
    bool                      _muteIt = true;
    Ufe::Selection            _savedSn;
};

class LAYEREDITOR_PUBLIC LockLayerCmd : public BaseCmd
{
public:
    LockLayerCmd(
        const PXR_NS::UsdStageRefPtr& stage,
        const PXR_NS::SdfLayerRefPtr& layer,
        LayerLockType              lockState,
        bool                       includeSubLayers = false,
        bool                       skipSystemLockedLayers = false)
        : BaseCmd(CmdId::kLockLayer, layer)
        , _lockType(lockState)
        , _includeSublayers(includeSubLayers)
        , _skipSystemLockedLayers(skipSystemLockedLayers)
        , _stage(stage)
    {
    }

    bool doIt(const PXR_NS::SdfLayerHandle& layer) override;

    bool undoIt(const PXR_NS::SdfLayerHandle& layer) override;

    // Sets whether or not the command should update the edit target upon completion.
    void SetUpdateEditTarget(bool updateEditTarget) { _updateEditTarget = updateEditTarget; }

    std::string commandString() const override
    {
        return _lockType == LayerLock_Unlocked ? "Unlock" : "Lock";
    }

private:
    PXR_NS::UsdStageWeakPtr getStage();

    LayerLockType              _lockType = LayerLockType::LayerLock_Locked;
    bool                       _includeSublayers = false;
    bool                       _skipSystemLockedLayers = false;
    bool                       _updateEditTarget = true;
    const PXR_NS::UsdStageRefPtr  _stage = nullptr;
    std::vector<LayerLockType> _previousStates;
    PXR_NS::SdfLayerHandleVector  _layers;
};

class LAYEREDITOR_PUBLIC InsertRemoveSubPathBaseCmd : public BaseCmd
{
public:
    InsertRemoveSubPathBaseCmd(
        CmdId                      id,
        const PXR_NS::UsdStageRefPtr& stage,
        const PXR_NS::SdfLayerRefPtr& layer,
        const std::string&         subpath,
        int                        index);

    bool doIt(const PXR_NS::SdfLayerHandle& layer) override;

    bool undoIt(const PXR_NS::SdfLayerHandle& layer) override;

    static bool validateUndoIndex(const PXR_NS::SdfLayerHandle& layer, int index);

    static bool validateAndReportIndex(const PXR_NS::SdfLayerHandle& layer, int index, int maxIndex);

    void saveSelection();

    void restoreSelection();

protected:
    std::string         _editTargetPath;
    Ufe::Selection      _savedSn;
    PXR_NS::UsdStageRefPtr _stage;
    std::string         _subPath;
    int                 _index = -1;
};

class LAYEREDITOR_PUBLIC InsertSubPathCmd : public InsertRemoveSubPathBaseCmd
{
public:
    InsertSubPathCmd(
        const PXR_NS::UsdStageRefPtr& stage,
        const PXR_NS::SdfLayerRefPtr& layer,
        const std::string&         subPath,
        int                        index)
        : InsertRemoveSubPathBaseCmd(CmdId::kInsert, stage, layer, subPath, index)
    {
    }

    std::string commandString() const override { return "Insert USD sublayer"; }
};

class LAYEREDITOR_PUBLIC RemoveSubPathCmd : public InsertRemoveSubPathBaseCmd
{
public:
    RemoveSubPathCmd(
        const PXR_NS::UsdStageRefPtr& stage,
        const PXR_NS::SdfLayerRefPtr& layer,
        const int                  index)
        : InsertRemoveSubPathBaseCmd(CmdId::kRemove, stage, layer, {}, index)
    {
    }

    RemoveSubPathCmd(
        const PXR_NS::UsdStageRefPtr& stage,
        const PXR_NS::SdfLayerRefPtr& layer,
        const std::string&         subpath)
        : InsertRemoveSubPathBaseCmd(CmdId::kRemove, stage, layer, subpath, -1)
    {
    }

    std::string commandString() const override { return "Remove USD sublayer"; }
};

class LAYEREDITOR_PUBLIC ReplaceSubPathCmd : public BaseCmd
{
public:
    ReplaceSubPathCmd(
        const PXR_NS::SdfLayerRefPtr& layer,
        const std::string&         oldPath,
        const std::string&         newPath)
        : BaseCmd(CmdId::kReplace, layer)
        , _oldPath(oldPath)
        , _newPath(newPath)
    {
    }

    bool        doIt(const PXR_NS::SdfLayerHandle& layer) override;
    bool        undoIt(const PXR_NS::SdfLayerHandle& layer) override;
    std::string commandString() const override { return "Replace USD sublayer path"; }

private:
    std::string _oldPath;
    std::string _newPath;
};

class LAYEREDITOR_PUBLIC MoveSubPathCmd : public BaseCmd
{
public:
    MoveSubPathCmd(
        const PXR_NS::SdfLayerRefPtr& layer,
        const PXR_NS::SdfLayerRefPtr& newParentLayer,
        const std::string&         subPath,
        int                        newIndex)
        : BaseCmd(CmdId::kMove, layer)
        , _newParent(newParentLayer)
        , _subPath(subPath)
        , _newIndex(newIndex)
    {
    }

    bool        doIt(const PXR_NS::SdfLayerHandle& layer) override;
    bool        undoIt(const PXR_NS::SdfLayerHandle& layer) override;
    std::string commandString() const override { return "Move USD sublayer"; }

private:
    PXR_NS::SdfLayerRefPtr _newParent;
    std::string         _subPath;
    std::string         _newPath;
    int                 _newIndex;
    int                 _oldIndex { -1 };
};

class LAYEREDITOR_PUBLIC RefreshSystemLockLayerCmd : public BaseCmd
{
public:
    RefreshSystemLockLayerCmd(
        const PXR_NS::UsdStageRefPtr& stage,
        const PXR_NS::SdfLayerHandle& layer,
        bool                       refreshSubLayers)
        : BaseCmd(CmdId::kRefreshSystemLock, layer)
        , _refreshSubLayers(refreshSubLayers)
        , _stage(stage)
    {
    }

    bool doIt(const PXR_NS::SdfLayerHandle& layer) override;

    // The command itself doesn't retain its state. However, the underlying logic contains commands
    // that are undoable.
    bool undoIt(const PXR_NS::SdfLayerHandle& layer) override;

    std::string commandString() const override { return "Refresh System Lock"; }

    // Adds a DCC-specific entry (e.g. "proxyShapePath") to the context sent with
    // the onRefreshSystemLock UI callback.
    void addCallbackContext(const std::string& key, const PXR_NS::VtValue& value);

    const PXR_NS::VtDictionary& extraCallbackContext() const { return _extraCallbackContext; }

private:
    std::string _quote(const std::string& string);

    PXR_NS::VtDictionary _extraCallbackContext;

    // Checks if the file layer or its sublayers are accessible on disk, and adds the layer
    // to _layers along with the _lockCommands to update the system-lock status.
    void _refreshLayerSystemLock(const PXR_NS::SdfLayerHandle& usdLayer);

    void _notifySystemLockIsRefreshed();

    bool                                       _refreshSubLayers = false;
    std::vector<std::shared_ptr<LockLayerCmd>> _lockCommands;
    PXR_NS::SdfLayerHandleVector                  _layers;
    PXR_NS::UsdStageWeakPtr                       _stage;
};

class LAYEREDITOR_PUBLIC StitchLayersCmd : public BackupLayerBaseCmd
{
public:
    StitchLayersCmd(
        const PXR_NS::UsdStageRefPtr&      stage,
        const std::vector<std::string>& layerIdentifiers)
        : BackupLayerBaseCmd(CmdId::kStitchLayers, PXR_NS::SdfLayerRefPtr())
        , _layerIdentifiersByStrength(layerIdentifiers)
        , _stage(stage)
    {
    }

    bool doIt(const PXR_NS::SdfLayerHandle& layer) override;

    bool undoIt(const PXR_NS::SdfLayerHandle& layer) override;

    std::string commandString() const override { return "Stitch Layers"; }

private:
    UsdUfe::UsdUndoableItem  _undoItem;
    std::vector<std::string> _layerIdentifiersByStrength;
    PXR_NS::UsdStageRefPtr      _stage;
};

class LAYEREDITOR_PUBLIC AddAnonSubLayerCmd : public InsertRemoveSubPathBaseCmd
{
public:
    AddAnonSubLayerCmd(const PXR_NS::UsdStageRefPtr& stage, const PXR_NS::SdfLayerRefPtr& layer)
        : InsertRemoveSubPathBaseCmd(CmdId::kAddAnonLayer, stage, layer, "", -1) {};

    bool doIt(const PXR_NS::SdfLayerHandle& layer) override
    {
        // the first time, USD will create a layer with a certain identifier
        // on undo(), we will remove the path, but hold onto the layer
        // on redo, we want to put back that same identifier, for later commands
        if (_anonIdentifier.empty()) {
            _anonLayer = PXR_NS::SdfLayer::CreateAnonymous(_anonName);
            _anonIdentifier = _anonLayer->GetIdentifier();
        }
        _subPath = _anonIdentifier;
        _index = 0;
        _cmdResult = _subPath;
        return InsertRemoveSubPathBaseCmd::doIt(layer);
    }

    bool undoIt(const PXR_NS::SdfLayerHandle& layer) override
    {
        return InsertRemoveSubPathBaseCmd::undoIt(layer);
    }

    std::string addedLayer() { return _subPath; }

    std::string commandString() const override { return "Add Anonymous Sublayer"; }

    std::string _anonName;

protected:
    PXR_NS::SdfLayerRefPtr _anonLayer;
    std::string         _anonIdentifier;
};

} // namespace UsdLayerEditor

#endif // LAYER_EDITOR_COMMANDS_H