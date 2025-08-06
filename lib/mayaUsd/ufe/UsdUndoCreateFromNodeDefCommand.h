//
// Copyright 2022 Autodesk
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
#ifndef MAYAUSD_USDUNDOCREATEFROMNODEDEFCOMMAND_H
#define MAYAUSD_USDUNDOCREATEFROMNODEDEFCOMMAND_H

#include <mayaUsd/base/api.h>

#include <usdUfe/ufe/UfeVersionCompat.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/UsdUndoAddNewPrimCommand.h>
#include <usdUfe/undo/UsdUndoableItem.h>

#include <pxr/usd/sdr/shaderNode.h>

#include <ufe/hierarchy.h>
#include <ufe/pathComponent.h>
#include <ufe/selection.h>
#include <ufe/undoableCommand.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief UsdUndoCreateFromNodeDefCommand
class MAYAUSD_CORE_PUBLIC UsdUndoCreateFromNodeDefCommand : public Ufe::InsertChildCommand
{
public:
    typedef std::shared_ptr<UsdUndoCreateFromNodeDefCommand> Ptr;

    UsdUndoCreateFromNodeDefCommand(
        const PXR_NS::SdrShaderNodeConstPtr shaderNodeDef,
        const UsdUfe::UsdSceneItem::Ptr&    parentItem,
        const Ufe::PathComponent&           name);

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoCreateFromNodeDefCommand);

    //! Create a UsdUndoCreateFromNodeDefCommand from a USD scene item and a UFE path component.
    static UsdUndoCreateFromNodeDefCommand::Ptr create(
        const PXR_NS::SdrShaderNodeConstPtr shaderNodeDef,
        const UsdUfe::UsdSceneItem::Ptr&    parentItem,
        const Ufe::PathComponent&           name);

    Ufe::SceneItem::Ptr insertedChild() const override;

    void execute() override;
    void undo() override;
    void redo() override;
    UFE_V4(std::string commandString() const override { return "CreateFromNodeDef"; })

private:
    const PXR_NS::SdrShaderNodeConstPtr _shaderNodeDef;

    std::shared_ptr<UsdUfe::UsdUndoAddNewPrimCommand> _addPrimCmd;

    UsdUfe::UsdUndoableItem _undoableItem;

    void setIdAttr();
}; // UsdUndoCreateFromNodeDefCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_USDUNDOCREATEFROMNODEDEFCOMMAND_H
