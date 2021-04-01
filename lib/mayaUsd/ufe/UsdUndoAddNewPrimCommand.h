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
#ifndef USD_ADD_NEW_PRIM_COMMAND
#define USD_ADD_NEW_PRIM_COMMAND

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdSceneItem.h>
#include <mayaUsd/undo/UsdUndoableItem.h>

#include <ufe/path.h>
#include <ufe/undoableCommand.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Undoable command for add new prim
class MAYAUSD_CORE_PUBLIC UsdUndoAddNewPrimCommand : public Ufe::UndoableCommand
{
public:
    typedef std::shared_ptr<UsdUndoAddNewPrimCommand> Ptr;

    UsdUndoAddNewPrimCommand(
        const UsdSceneItem::Ptr& usdSceneItem,
        const std::string&       name,
        const std::string&       type);

    void execute() override;
    void undo() override;
    void redo() override;

    const Ufe::Path& newUfePath() const;
    PXR_NS::UsdPrim  newPrim() const;

    static UsdUndoAddNewPrimCommand::Ptr
    create(const UsdSceneItem::Ptr& usdSceneItem, const std::string& name, const std::string& type);

private:
    PXR_NS::UsdStageWeakPtr _stage;
    PXR_NS::SdfPath         _primPath;
    PXR_NS::TfToken         _primToken;
    Ufe::Path               _newUfePath;
    UsdUndoableItem         _undoableItem;

}; // UsdUndoAddNewPrimCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif
