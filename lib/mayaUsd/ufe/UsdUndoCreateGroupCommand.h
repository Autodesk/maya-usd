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

// PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief UsdUndoCreateGroupCommand
class MAYAUSD_CORE_PUBLIC UsdUndoCreateGroupCommand : public Ufe::CompositeUndoableCommand
{
public:
    typedef std::shared_ptr<UsdUndoCreateGroupCommand> Ptr;

    UsdUndoCreateGroupCommand(
        const UsdSceneItem::Ptr&  parentItem,
        const Ufe::Selection&     selection,
        const Ufe::PathComponent& name);
    ~UsdUndoCreateGroupCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdUndoCreateGroupCommand(const UsdUndoCreateGroupCommand&) = delete;
    UsdUndoCreateGroupCommand& operator=(const UsdUndoCreateGroupCommand&) = delete;
    UsdUndoCreateGroupCommand(UsdUndoCreateGroupCommand&&) = delete;
    UsdUndoCreateGroupCommand& operator=(UsdUndoCreateGroupCommand&&) = delete;

    //! Create a UsdUndoCreateGroupCommand from a USD scene item and a UFE path component.
    static UsdUndoCreateGroupCommand::Ptr create(
        const UsdSceneItem::Ptr&  parentItem,
        const Ufe::Selection&     selection,
        const Ufe::PathComponent& name);
    Ufe::SceneItem::Ptr group() const;

    // UsdUndoCreateGroupCommand overrides
    void execute() override;

private:
    UsdSceneItem::Ptr  _parentItem;
    Ufe::PathComponent _name;
    UsdSceneItem::Ptr  _group;
    Ufe::Selection     _selection;

}; // UsdUndoCreateGroupCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
