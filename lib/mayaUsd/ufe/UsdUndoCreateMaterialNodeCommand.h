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
#include <mayaUsd/ufe/UsdSceneItem.h>

#include <ufe/pathComponent.h>
#include <ufe/selection.h>
#include <ufe/undoableCommand.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief UsdUndoCreateMaterialNodeCommand
class MAYAUSD_CORE_PUBLIC UsdUndoCreateMaterialNodeCommand : public Ufe::InsertChildCommand
{
public:
    typedef std::shared_ptr<UsdUndoCreateMaterialNodeCommand> Ptr;

    UsdUndoCreateMaterialNodeCommand(
        const UsdSceneItem::Ptr& parentItem,
        const std::string& materialBaseName,
        const std::string& materialNodeType,
        const Ufe::PathComponent& materialNodeName);
    ~UsdUndoCreateMaterialNodeCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdUndoCreateMaterialNodeCommand(const UsdUndoCreateMaterialNodeCommand&) = delete;
    UsdUndoCreateMaterialNodeCommand& operator=(const UsdUndoCreateMaterialNodeCommand&) = delete;
    UsdUndoCreateMaterialNodeCommand(UsdUndoCreateMaterialNodeCommand&&) = delete;
    UsdUndoCreateMaterialNodeCommand& operator=(UsdUndoCreateMaterialNodeCommand&&) = delete;

    //! Create a UsdUndoCreateGroupCommand from a USD scene item and a UFE path component.
    static UsdUndoCreateMaterialNodeCommand::Ptr create(
        const UsdSceneItem::Ptr& parentItem,
        const std::string& materialBaseName,
        const std::string& materialNodeType,
        const Ufe::PathComponent& materialNodeName);

    Ufe::SceneItem::Ptr insertedChild() const override;

    void execute() override;
    void undo() override;
    void redo() override;

private:
    UsdSceneItem::Ptr  _parentItem;
    const std::string _materialBaseName;
    const std::string _materialNodeType;
    const Ufe::PathComponent _materialNodeName;
    UsdSceneItem::Ptr  _materialItem;
    Ufe::SceneItem::Ptr  _materialNodeItem;

    std::shared_ptr<Ufe::CompositeUndoableCommand> _materialNodeCompositeCmd;

}; // UsdUndoCreateMaterialNodeCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
