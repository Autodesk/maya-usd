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
#ifndef USDUFE_USDUNDOCREATEGROUPCOMMAND_H
#define USDUFE_USDUNDOCREATEGROUPCOMMAND_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UfeVersionCompat.h>
#include <usdUfe/ufe/UsdSceneItem.h>

#include <ufe/hierarchy.h>
#include <ufe/pathComponent.h>
#include <ufe/selection.h>
#include <ufe/undoableCommand.h>

namespace USDUFE_NS_DEF {

//! \brief UsdUndoCreateGroupCommand
class USDUFE_PUBLIC UsdUndoCreateGroupCommand : public Ufe::InsertChildCommand
{
public:
    typedef std::shared_ptr<UsdUndoCreateGroupCommand> Ptr;

    UsdUndoCreateGroupCommand(
        const UsdSceneItem::Ptr& parentItem,
#ifndef UFE_V3_FEATURES_AVAILABLE
        const Ufe::Selection& selection,
#endif
        const Ufe::PathComponent& name);

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoCreateGroupCommand);

    //! Create a UsdUndoCreateGroupCommand from a USD scene item and a UFE path component.
    static UsdUndoCreateGroupCommand::Ptr create(
        const UsdSceneItem::Ptr& parentItem,
#ifndef UFE_V3_FEATURES_AVAILABLE
        const Ufe::Selection& selection,
#endif
        const Ufe::PathComponent& name);

    Ufe::SceneItem::Ptr insertedChild() const override;

    void execute() override;
    void undo() override;
    void redo() override;
    UFE_V4(std::string commandString() const override { return "CreateGroup"; })

private:
    UsdSceneItem::Ptr  _parentItem;
    Ufe::PathComponent _name;
    UsdSceneItem::Ptr  _groupItem;
#ifndef UFE_V3_FEATURES_AVAILABLE
    Ufe::Selection _selection;
#endif

    std::shared_ptr<Ufe::CompositeUndoableCommand> _groupCompositeCmd;

}; // UsdUndoCreateGroupCommand

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDUNDOCREATEGROUPCOMMAND_H
