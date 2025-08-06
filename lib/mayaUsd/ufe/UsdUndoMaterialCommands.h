//
// Copyright 2019 Autodesk
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
#ifndef MAYAUSD_USDUNDOMATERIALCOMMANDS_H
#define MAYAUSD_USDUNDOMATERIALCOMMANDS_H

#include <mayaUsd/base/api.h>

#include <usdUfe/ufe/UfeVersionCompat.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/undo/UsdUndoableItem.h>
#ifdef UFE_V4_FEATURES_AVAILABLE
#include <mayaUsd/ufe/UsdUndoCreateFromNodeDefCommand.h>

#include <usdUfe/ufe/UsdUndoAddNewPrimCommand.h>
#endif

#include <pxr/usd/usd/prim.h>

#include <ufe/path.h>
#include <ufe/pathComponent.h>
#include <ufe/selection.h>
#include <ufe/undoableCommand.h>

#include <map>
#include <vector>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief BindMaterialUndoableCommand
class MAYAUSD_CORE_PUBLIC BindMaterialUndoableCommand : public Ufe::UndoableCommand
{
public:
    static const std::string commandName;

    static bool CompatiblePrim(const Ufe::SceneItem::Ptr& item);

    BindMaterialUndoableCommand(Ufe::Path primPath, const PXR_NS::SdfPath& materialPath);

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(BindMaterialUndoableCommand);

    void execute() override;
    void undo() override;
    void redo() override;
    UFE_V4(std::string commandString() const override { return "BindMaterial"; })

private:
    Ufe::Path               _primPath;
    PXR_NS::SdfPath         _materialPath;
    UsdUfe::UsdUndoableItem _undoableItem;
};

//! \brief UnbindMaterialUndoableCommand
class MAYAUSD_CORE_PUBLIC UnbindMaterialUndoableCommand : public Ufe::UndoableCommand
{
public:
    static const std::string commandName;

    UnbindMaterialUndoableCommand(Ufe::Path primPath);
    ~UnbindMaterialUndoableCommand() override;

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UnbindMaterialUndoableCommand);

    void execute() override;
    void undo() override;
    void redo() override;
    UFE_V4(std::string commandString() const override { return "UnbindMaterial"; })

private:
    Ufe::Path               _primPath;
    UsdUfe::UsdUndoableItem _undoableItem;
};

#ifdef UFE_V4_FEATURES_AVAILABLE
//! \brief UsdUndoAssignNewMaterialCommand
class MAYAUSD_CORE_PUBLIC UsdUndoAssignNewMaterialCommand : public Ufe::InsertChildCommand
{
public:
    typedef std::shared_ptr<UsdUndoAssignNewMaterialCommand> Ptr;

    UsdUndoAssignNewMaterialCommand(
        const UsdUfe::UsdSceneItem::Ptr& parentItem,
        const std::string&               sdrShaderIdentifier);
    UsdUndoAssignNewMaterialCommand(
        const Ufe::Selection& parentItems,
        const std::string&    sdrShaderIdentifier);
    ~UsdUndoAssignNewMaterialCommand() override;

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoAssignNewMaterialCommand);

    //! Create a UsdUndoAssignNewMaterialCommand that creates a new material based on
    //! \p sdrShaderIdentifier and assigns it to \p parentItem
    static UsdUndoAssignNewMaterialCommand::Ptr
    create(const UsdUfe::UsdSceneItem::Ptr& parentItem, const std::string& sdrShaderIdentifier);
    //! Create a UsdUndoAssignNewMaterialCommand that creates a new material based on
    //! \p sdrShaderIdentifier and assigns it to multiple \p parentItems
    static UsdUndoAssignNewMaterialCommand::Ptr
    create(const Ufe::Selection& parentItems, const std::string& sdrShaderIdentifier);

    Ufe::SceneItem::Ptr insertedChild() const override;

    void execute() override;
    void undo() override;
    void redo() override;
    UFE_V4(std::string commandString() const override { return "AssignNewMaterial"; })

private:
    void markAsFailed();

    std::map<PXR_NS::UsdStageWeakPtr, std::vector<Ufe::Path>> _stagesAndPaths;
    const std::string                                         _nodeId;

    size_t                                         _createMaterialCmdIdx = -1;
    std::shared_ptr<Ufe::CompositeUndoableCommand> _cmds;

}; // UsdUndoAssignNewMaterialCommand

//! \brief UsdUndoAddNewMaterialCommand
class MAYAUSD_CORE_PUBLIC UsdUndoAddNewMaterialCommand : public Ufe::InsertChildCommand
{
public:
    typedef std::shared_ptr<UsdUndoAddNewMaterialCommand> Ptr;

    UsdUndoAddNewMaterialCommand(
        const UsdUfe::UsdSceneItem::Ptr& parentItem,
        const std::string&               sdrShaderIdentifier);
    ~UsdUndoAddNewMaterialCommand() override;

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoAddNewMaterialCommand);

    //! Create a UsdUndoAddNewMaterialCommand that creates a new material based on
    //! \p sdrShaderIdentifier and adds it as child of \p parentItem
    static UsdUndoAddNewMaterialCommand::Ptr
    create(const UsdUfe::UsdSceneItem::Ptr& parentItem, const std::string& sdrShaderIdentifier);

    Ufe::SceneItem::Ptr insertedChild() const override;

    void execute() override;
    void undo() override;
    void redo() override;
    UFE_V4(std::string commandString() const override { return "AddNewMaterial"; })

    // Can we add a material to this item.
    static bool CompatiblePrim(const Ufe::SceneItem::Ptr& target);

private:
    void markAsFailed();

    Ufe::Path         _parentPath;
    const std::string _nodeId;

    UsdUfe::UsdUndoAddNewPrimCommand::Ptr _createMaterialCmd;
    UsdUndoCreateFromNodeDefCommand::Ptr  _createShaderCmd;

}; // UsdUndoAddNewMaterialCommand

//! \brief This command is used to create a materials scope under a specified parent item. A
//! materials scope is a USD Scope with a special name (usually "mtl"), which holds materials. By
//! convention, all materials should reside within such a scope.
class MAYAUSD_CORE_PUBLIC UsdUndoCreateMaterialsScopeCommand
    : public Ufe::SceneItemResultUndoableCommand
{
public:
    typedef std::shared_ptr<UsdUndoCreateMaterialsScopeCommand> Ptr;

    UsdUndoCreateMaterialsScopeCommand(const UsdUfe::UsdSceneItem::Ptr& parentItem);
    ~UsdUndoCreateMaterialsScopeCommand() override;

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoCreateMaterialsScopeCommand);

    //! Create a UsdUndoCreateMaterialsScopeCommand that creates a new materials scope under \p
    //! parentItem. If there already is a materials scope under \p parentItem, the command will
    //! not create a new materials scope but simply point to the existing one.
    static UsdUndoCreateMaterialsScopeCommand::Ptr
    create(const UsdUfe::UsdSceneItem::Ptr& parentItem);

    Ufe::SceneItem::Ptr sceneItem() const override;

    void execute() override;
    void undo() override;
    void redo() override;
    UFE_V4(std::string commandString() const override { return "CreateMaterialsScope"; })

private:
    bool doExecute();
    void markAsFailed();

    UsdUfe::UsdSceneItem::Ptr _parentItem;
    Ufe::SceneItem::Ptr       _insertedChild;
    UsdUfe::UsdUndoableItem   _undoableItem;
}; // UsdUndoCreateMaterialsScopeCommand
#endif

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_USDUNDOMATERIALCOMMANDS_H
