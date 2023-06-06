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
#pragma once

#include <mayaUsd/base/api.h>

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
    ~BindMaterialUndoableCommand() override;

    // Delete the copy/move constructors assignment operators.
    BindMaterialUndoableCommand(const BindMaterialUndoableCommand&) = delete;
    BindMaterialUndoableCommand& operator=(const BindMaterialUndoableCommand&) = delete;
    BindMaterialUndoableCommand(BindMaterialUndoableCommand&&) = delete;
    BindMaterialUndoableCommand& operator=(BindMaterialUndoableCommand&&) = delete;

    void execute() override;
    void undo() override;
    void redo() override;

private:
    Ufe::Path       _primPath;
    PXR_NS::SdfPath _materialPath;
    UsdUndoableItem _undoableItem;
};

//! \brief UnbindMaterialUndoableCommand
class MAYAUSD_CORE_PUBLIC UnbindMaterialUndoableCommand : public Ufe::UndoableCommand
{
public:
    static const std::string commandName;

    UnbindMaterialUndoableCommand(Ufe::Path primPath);
    ~UnbindMaterialUndoableCommand() override;

    // Delete the copy/move constructors assignment operators.
    UnbindMaterialUndoableCommand(const UnbindMaterialUndoableCommand&) = delete;
    UnbindMaterialUndoableCommand& operator=(const UnbindMaterialUndoableCommand&) = delete;
    UnbindMaterialUndoableCommand(UnbindMaterialUndoableCommand&&) = delete;
    UnbindMaterialUndoableCommand& operator=(UnbindMaterialUndoableCommand&&) = delete;

    void execute() override;
    void undo() override;
    void redo() override;

private:
    Ufe::Path       _primPath;
    UsdUndoableItem _undoableItem;
};

#ifdef UFE_V4_FEATURES_AVAILABLE
//! \brief UsdUndoAssignNewMaterialCommand
class MAYAUSD_CORE_PUBLIC UsdUndoAssignNewMaterialCommand : public Ufe::InsertChildCommand
{
public:
    typedef std::shared_ptr<UsdUndoAssignNewMaterialCommand> Ptr;

    UsdUndoAssignNewMaterialCommand(
        const UsdSceneItem::Ptr& parentItem,
        const std::string&       sdrShaderIdentifier);
    UsdUndoAssignNewMaterialCommand(
        const Ufe::Selection& parentItems,
        const std::string&    sdrShaderIdentifier);
    ~UsdUndoAssignNewMaterialCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdUndoAssignNewMaterialCommand(const UsdUndoAssignNewMaterialCommand&) = delete;
    UsdUndoAssignNewMaterialCommand& operator=(const UsdUndoAssignNewMaterialCommand&) = delete;
    UsdUndoAssignNewMaterialCommand(UsdUndoAssignNewMaterialCommand&&) = delete;
    UsdUndoAssignNewMaterialCommand& operator=(UsdUndoAssignNewMaterialCommand&&) = delete;

    //! Create a UsdUndoAssignNewMaterialCommand that creates a new material based on
    //! \p sdrShaderIdentifier and assigns it to \p parentItem
    static UsdUndoAssignNewMaterialCommand::Ptr
    create(const UsdSceneItem::Ptr& parentItem, const std::string& sdrShaderIdentifier);
    //! Create a UsdUndoAssignNewMaterialCommand that creates a new material based on
    //! \p sdrShaderIdentifier and assigns it to multiple \p parentItems
    static UsdUndoAssignNewMaterialCommand::Ptr
    create(const Ufe::Selection& parentItems, const std::string& sdrShaderIdentifier);

    Ufe::SceneItem::Ptr insertedChild() const override;

    void execute() override;
    void undo() override;
    void redo() override;

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
        const UsdSceneItem::Ptr& parentItem,
        const std::string&       sdrShaderIdentifier);
    ~UsdUndoAddNewMaterialCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdUndoAddNewMaterialCommand(const UsdUndoAddNewMaterialCommand&) = delete;
    UsdUndoAddNewMaterialCommand& operator=(const UsdUndoAddNewMaterialCommand&) = delete;
    UsdUndoAddNewMaterialCommand(UsdUndoAddNewMaterialCommand&&) = delete;
    UsdUndoAddNewMaterialCommand& operator=(UsdUndoAddNewMaterialCommand&&) = delete;

    //! Create a UsdUndoAddNewMaterialCommand that creates a new material based on
    //! \p sdrShaderIdentifier and adds it as child of \p parentItem
    static UsdUndoAddNewMaterialCommand::Ptr
    create(const UsdSceneItem::Ptr& parentItem, const std::string& sdrShaderIdentifier);

    Ufe::SceneItem::Ptr insertedChild() const override;

    void execute() override;
    void undo() override;
    void redo() override;

    // Can we add a material to this item.
    static bool CompatiblePrim(const Ufe::SceneItem::Ptr& target);

private:
    void markAsFailed();

    Ufe::Path         _parentPath;
    const std::string _nodeId;

    UsdUndoAddNewPrimCommand::Ptr        _createMaterialCmd;
    UsdUndoCreateFromNodeDefCommand::Ptr _createShaderCmd;

}; // UsdUndoAddNewMaterialCommand

//! \brief This command is used to create a materials scope under a specified parent item. A
//! materials scope is a USD Scope with a special name (usually "mtl"), which holds materials. By
//! convention, all materials should reside within such a scope.
class MAYAUSD_CORE_PUBLIC UsdUndoCreateMaterialsScopeCommand
    : public Ufe::SceneItemResultUndoableCommand
{
public:
    typedef std::shared_ptr<UsdUndoCreateMaterialsScopeCommand> Ptr;

    UsdUndoCreateMaterialsScopeCommand(const UsdSceneItem::Ptr& parentItem);
    ~UsdUndoCreateMaterialsScopeCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdUndoCreateMaterialsScopeCommand(const UsdUndoCreateMaterialsScopeCommand&) = delete;
    UsdUndoCreateMaterialsScopeCommand& operator=(const UsdUndoCreateMaterialsScopeCommand&)
        = delete;
    UsdUndoCreateMaterialsScopeCommand(UsdUndoCreateMaterialsScopeCommand&&) = delete;
    UsdUndoCreateMaterialsScopeCommand& operator=(UsdUndoCreateMaterialsScopeCommand&&) = delete;

    //! Create a UsdUndoCreateMaterialsScopeCommand that creates a new materials scope under \p
    //! parentItem. If there already is a materials scope under \p parentItem, the command will
    //! not create a new materials scope but simply point to the existing one.
    static UsdUndoCreateMaterialsScopeCommand::Ptr create(const UsdSceneItem::Ptr& parentItem);

    Ufe::SceneItem::Ptr sceneItem() const override;

    void execute() override;
    void undo() override;
    void redo() override;

private:
    void markAsFailed();

    UsdSceneItem::Ptr   _parentItem;
    Ufe::SceneItem::Ptr _insertedChild;

    UsdUndoableItem _undoableItem;
}; // UsdUndoCreateMaterialsScopeCommand
#endif

} // namespace ufe
} // namespace MAYAUSD_NS_DEF