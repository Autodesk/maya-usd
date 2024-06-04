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
#ifndef USDUFE_USDUNDOINSERTCHILDCOMMAND_H
#define USDUFE_USDUNDOINSERTCHILDCOMMAND_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UfeVersionCompat.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/undo/UsdUndoableItem.h>

#include <pxr/usd/usd/prim.h>

#include <ufe/hierarchy.h>
#include <ufe/selection.h>

namespace USDUFE_NS_DEF {

//! \brief UsdUndoInsertChildCommand
class USDUFE_PUBLIC UsdUndoInsertChildCommand : public Ufe::InsertChildCommand
{
public:
    using Ptr = std::shared_ptr<UsdUndoInsertChildCommand>;

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoInsertChildCommand);

    //! Create a UsdUndoInsertChildCommand.  Note that as of 4-May-2020 the
    //! pos argument is ignored, and only append is supported.
    static UsdUndoInsertChildCommand::Ptr create(
        const UsdSceneItem::Ptr& parent,
        const UsdSceneItem::Ptr& child,
        const UsdSceneItem::Ptr& pos);

    Ufe::SceneItem::Ptr insertedChild() const override { return _ufeDstItem; }

    UFE_V4(std::string commandString() const override;)

protected:
    //! Construct a UsdUndoInsertChildCommand.  Note that as of 4-May-2020 the
    //! pos argument is ignored, and only append is supported.
    UsdUndoInsertChildCommand(
        const UsdSceneItem::Ptr& parent,
        const UsdSceneItem::Ptr& child,
        const UsdSceneItem::Ptr& pos);

private:
    void execute() override;
    void undo() override;
    void redo() override;

    UsdSceneItem::Ptr _ufeDstItem;

    Ufe::Path _ufeSrcPath;
    Ufe::Path _ufeParentPath;
    Ufe::Path _ufeDstPath;

    PXR_NS::SdfPath _usdSrcPath;
    PXR_NS::SdfPath _usdDstPath;

    UsdUndoableItem _undoableItem;
}; // UsdUndoInsertChildCommand

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDUNDOINSERTCHILDCOMMAND_H
