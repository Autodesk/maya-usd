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

#include "abstractCommandHook.h"
#include "layerEditorAPI.h"

#include <usdUfe/undo/UsdUndoableItem.h>

#include <ufe/selection.h>
#include <ufe/undoableCommand.h>

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

class LayerEditorAPI BaseCmd : public Ufe::UndoableCommand
{
public:
    BaseCmd(CmdId id, const pxr::SdfLayerRefPtr& layer)
        : _cmdId(id)
        , _layer(layer)
    {
    }
    virtual ~BaseCmd() { }

    void execute() override { redo(); }
    void undo() override;
    void redo() override;

    virtual bool doIt(const pxr::SdfLayerHandle& layer) = 0;
    virtual bool undoIt(const pxr::SdfLayerHandle& layer) = 0;

protected:
    CmdId               _cmdId;
    std::string         _cmdResult; // set if the command returns something
    pxr::SdfLayerRefPtr _layer;

    // we need to hold on to dirty sublayers if we remove them
    std::vector<PXR_NS::SdfLayerRefPtr> _subLayersRefs;
    void                                holdOntoSubLayers(const pxr::SdfLayerHandle& layer);
    void                                releaseSubLayers() { _subLayersRefs.clear(); }
    void holdOnPathIfDirty(const pxr::SdfLayerHandle& layer, const std::string& path);
    void updateEditTarget(const PXR_NS::UsdStageWeakPtr stage);
};

class LayerEditorAPI BackupLayerBaseCmd : public BaseCmd
{
    // commands that need to backup the whole layer for undo
public:
    BackupLayerBaseCmd(CmdId id, const pxr::SdfLayerRefPtr& layer)
        : BaseCmd(id, layer)
    {
    }

    bool doIt(const pxr::SdfLayerHandle& layer) override;

    bool undoIt(const pxr::SdfLayerHandle& layer) override;

protected:
    // Backup and restore edit targets of stages that were targeting the sub-layers
    // of the cleared layer to support undo and redo.
    void backupEditTargets(const pxr::SdfLayerHandle& layer);

    void restoreEditTargets();

private:
    // Backup dirty layer to support undo and redo.
    void backupLayer(const pxr::SdfLayerHandle& layer);

    void restoreLayer(const pxr::SdfLayerHandle& layer);

    // Edit targets that were made invalid after the layer was cleared.
    // The stages are kept with weak pointer to avoid forcing to stay valid.
    using EditTargetBackups = std::map<PXR_NS::UsdStagePtr, PXR_NS::UsdEditTarget>;
    EditTargetBackups _editTargetBackups;

    // we need to hold onto the layer if we dirty it
    PXR_NS::SdfLayerRefPtr _backupLayer;
};

class LayerEditorAPI ClearLayerCmd : public BackupLayerBaseCmd
{
public:
    ClearLayerCmd(const pxr::SdfLayerRefPtr& layer)
        : BackupLayerBaseCmd(CmdId::kClearLayer, layer)
    {
    }
    std::string commandString() const override { return "Clear USD layer"; }
};

class LayerEditorAPI FlattenLayerCmd : public BackupLayerBaseCmd
{
public:
    FlattenLayerCmd(const pxr::SdfLayerRefPtr& layer)
        : BackupLayerBaseCmd(CmdId::kFlattenLayer, layer)
    {
    }
    std::string commandString() const override { return "Merge with Sublayers"; }
};

class LayerEditorAPI DiscardEditCmd : public BackupLayerBaseCmd
{
public:
    DiscardEditCmd(const pxr::SdfLayerRefPtr& layer)
        : BackupLayerBaseCmd(CmdId::kDiscardEdit, layer)
    {
    }
    std::string commandString() const override { return "Reload USD layer"; }
};

class LayerEditorAPI SetEditTargetCmd : public Ufe::UndoableCommand
{

public:
    SetEditTargetCmd(const pxr::UsdStagePtr& stage, UsdLayerEditor::UsdLayer usdLayer)
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
    const pxr::UsdStagePtr   stage;
    UsdLayerEditor::UsdLayer newTarget;
    pxr::UsdEditTarget       oldTarget;
};

class LayerEditorAPI MuteLayerCmd : public BaseCmd
{
public:
    MuteLayerCmd(const pxr::UsdStageRefPtr& stage, const pxr::SdfLayerRefPtr& layer, bool muteIt)
        : BaseCmd(CmdId::kMuteLayer, layer)
        , _stage(stage)
        , _muteIt(muteIt)
    {
    }

    bool doIt(const pxr::SdfLayerHandle& layer) override;

    bool undoIt(const pxr::SdfLayerHandle& layer) override;

    std::string commandString() const override
    {
        return _muteIt ? "Mute" : "Unmute";
    }

private:
    pxr::UsdStageWeakPtr getStage();

    void saveSelection();

    void restoreSelection();

    const pxr::UsdStageRefPtr _stage = nullptr;
    bool                      _muteIt = true;
    Ufe::Selection            _savedSn;
};

class LayerEditorAPI LockLayerCmd : public BaseCmd
{
public:
    LockLayerCmd(
        const pxr::UsdStageRefPtr& stage,
        const pxr::SdfLayerRefPtr& layer,
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

    bool doIt(const pxr::SdfLayerHandle& layer) override;

    bool undoIt(const pxr::SdfLayerHandle& layer) override;

    // Sets whether or not the command should update the edit target upon completion.
    void SetUpdateEditTarget(bool updateEditTarget) { _updateEditTarget = updateEditTarget; }

    std::string commandString() const override
    {
        return _lockType == LayerLock_Unlocked ? "Unlock" : "Lock";
    }

private:
    pxr::UsdStageWeakPtr getStage();

    LayerLockType              _lockType = LayerLockType::LayerLock_Locked;
    bool                       _includeSublayers = false;
    bool                       _skipSystemLockedLayers = false;
    bool                       _updateEditTarget = true;
    const pxr::UsdStageRefPtr  _stage = nullptr;
    std::vector<LayerLockType> _previousStates;
    pxr::SdfLayerHandleVector  _layers;
};

class LayerEditorAPI InsertRemoveSubPathBaseCmd : public BaseCmd
{
public:
    InsertRemoveSubPathBaseCmd(
        CmdId                      id,
        const pxr::UsdStageRefPtr& stage,
        const pxr::SdfLayerRefPtr& layer,
        const std::string&         subpath,
        int                        index);

    bool doIt(const pxr::SdfLayerHandle& layer) override;

    bool undoIt(const pxr::SdfLayerHandle& layer) override;

    static bool validateUndoIndex(const pxr::SdfLayerHandle& layer, int index);

    static bool validateAndReportIndex(const pxr::SdfLayerHandle& layer, int index, int maxIndex);

    void saveSelection();

    void restoreSelection();

protected:
    std::string         _editTargetPath;
    Ufe::Selection      _savedSn;
    pxr::UsdStageRefPtr _stage;
    std::string         _subPath;
    int                 _index = -1;
};

class LayerEditorAPI InsertSubPathCmd : public InsertRemoveSubPathBaseCmd
{
public:
    InsertSubPathCmd(
        const pxr::UsdStageRefPtr& stage,
        const pxr::SdfLayerRefPtr& layer,
        const std::string&         subPath,
        int                        index)
        : InsertRemoveSubPathBaseCmd(CmdId::kInsert, stage, layer, subPath, index)
    {
    }

    std::string commandString() const override { return "Insert USD sublayer"; }
};

class LayerEditorAPI RemoveSubPathCmd : public InsertRemoveSubPathBaseCmd
{
public:
    // Constructor using subpath index.
    RemoveSubPathCmd(
        const pxr::UsdStageRefPtr& stage,
        const pxr::SdfLayerRefPtr& layer,
        const int                  index)
        : InsertRemoveSubPathBaseCmd(CmdId::kRemove, stage, layer, {}, index)
    {
    }

    // Constructor using subpath.
    RemoveSubPathCmd(
        const pxr::UsdStageRefPtr& stage,
        const pxr::SdfLayerRefPtr& layer,
        const std::string&         subpath)
        : InsertRemoveSubPathBaseCmd(CmdId::kRemove, stage, layer, subpath, -1)
    {
    }

    std::string commandString() const override { return "Remove USD sublayer"; }
};

class LayerEditorAPI RefreshSystemLockLayerCmd : public BaseCmd
{
public:
    RefreshSystemLockLayerCmd(
        const pxr::UsdStageRefPtr& stage,
        const pxr::SdfLayerHandle& layer,
        bool                       refreshSubLayers)
        : BaseCmd(CmdId::kRefreshSystemLock, layer)
        , _refreshSubLayers(refreshSubLayers)
        , _stage(stage)
    {
    }

    bool doIt(const pxr::SdfLayerHandle& layer) override;

    // The command itself doesn't retain its state. However, the underlying logic contains commands
    // that are undoable.
    bool undoIt(const pxr::SdfLayerHandle& layer) override;

    std::string commandString() const override { return "Refresh System Lock"; }

private:
    std::string _quote(const std::string& string);

    // Checks if the file layer or its sublayers are accessible on disk, and adds the layer
    // to _layers along with the _lockCommands to update the system-lock status.
    void _refreshLayerSystemLock(const pxr::SdfLayerHandle& usdLayer);

    void _notifySystemLockIsRefreshed();

    bool                                       _refreshSubLayers = false;
    std::vector<std::shared_ptr<LockLayerCmd>> _lockCommands;
    pxr::SdfLayerHandleVector                  _layers;
    pxr::UsdStageWeakPtr                       _stage;
};

class LayerEditorAPI StitchLayersCmd : public BackupLayerBaseCmd
{
public:
    StitchLayersCmd(
        const pxr::UsdStageRefPtr&      stage,
        const std::vector<std::string>& layerIdentifiers)
        : BackupLayerBaseCmd(CmdId::kStitchLayers, pxr::SdfLayerRefPtr())
        , _layerIdentifiersByStrength(layerIdentifiers)
        , _stage(stage)
    {
    }

    bool doIt(const pxr::SdfLayerHandle& layer) override;

    bool undoIt(const pxr::SdfLayerHandle& layer) override;

    std::string commandString() const override { return "Stitch Layers"; }

private:
    UsdUfe::UsdUndoableItem  _undoItem;
    std::vector<std::string> _layerIdentifiersByStrength;
    pxr::UsdStageRefPtr      _stage;
};

class LayerEditorAPI AddAnonSubLayerCmd : public InsertRemoveSubPathBaseCmd
{
public:
    AddAnonSubLayerCmd(const pxr::UsdStageRefPtr& stage,
        const pxr::SdfLayerRefPtr& layer)
        : InsertRemoveSubPathBaseCmd(CmdId::kAddAnonLayer, stage, layer, "", -1) {
    };

    bool doIt(const pxr::SdfLayerHandle& layer) override
    {
        // the first time, USD will create a layer with a certain identifier
        // on undo(), we will remove the path, but hold onto the layer
        // on redo, we want to put back that same identifier, for later commands
        if (_anonIdentifier.empty()) {
            _anonLayer = pxr::SdfLayer::CreateAnonymous(_anonName);
            _anonIdentifier = _anonLayer->GetIdentifier();
        }
        _subPath = _anonIdentifier;
        _index = 0;
        _cmdResult = _subPath;
        return InsertRemoveSubPathBaseCmd::doIt(layer);
    }

    bool undoIt(const pxr::SdfLayerHandle& layer) override { return InsertRemoveSubPathBaseCmd::undoIt(layer); }

    std::string addedLayer()
    {
        return _subPath;
    }

    std::string commandString() const override { return "Add Anonymous Sublayer"; }

    std::string _anonName;

protected:
    pxr::SdfLayerRefPtr _anonLayer;
    std::string         _anonIdentifier;
};

} // namespace UsdLayerEditor