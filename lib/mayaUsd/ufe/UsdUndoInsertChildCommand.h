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
#pragma once

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdSceneItem.h>

#include <pxr/usd/usd/prim.h>

#include <ufe/hierarchy.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief UsdUndoInsertChildCommand
class MAYAUSD_CORE_PUBLIC UsdUndoInsertChildCommand : public Ufe::InsertChildCommand
{
public:
    using Ptr = std::shared_ptr<UsdUndoInsertChildCommand>;

    ~UsdUndoInsertChildCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdUndoInsertChildCommand(const UsdUndoInsertChildCommand&) = delete;
    UsdUndoInsertChildCommand& operator=(const UsdUndoInsertChildCommand&) = delete;
    UsdUndoInsertChildCommand(UsdUndoInsertChildCommand&&) = delete;
    UsdUndoInsertChildCommand& operator=(UsdUndoInsertChildCommand&&) = delete;

    //! Create a UsdUndoInsertChildCommand.  Note that as of 4-May-2020 the
    //! pos argument is ignored, and only append is supported.
    static UsdUndoInsertChildCommand::Ptr create(
        const UsdSceneItem::Ptr& parent,
        const UsdSceneItem::Ptr& child,
        const UsdSceneItem::Ptr& pos);

    Ufe::SceneItem::Ptr insertedChild() const override { return _ufeDstItem; }

protected:
    //! Construct a UsdUndoInsertChildCommand.  Note that as of 4-May-2020 the
    //! pos argument is ignored, and only append is supported.
    UsdUndoInsertChildCommand(
        const UsdSceneItem::Ptr& parent,
        const UsdSceneItem::Ptr& child,
        const UsdSceneItem::Ptr& pos);

private:
    void undo() override;
    void redo() override;

    bool insertChildRedo();
    bool insertChildUndo();

    UsdSceneItem::Ptr _ufeDstItem;

    Ufe::Path _ufeSrcPath;
    Ufe::Path _ufeDstPath;

    SdfPath _usdSrcPath;
    SdfPath _usdDstPath;

    SdfLayerHandle _childLayer;
    SdfLayerHandle _parentLayer;

}; // UsdUndoInsertChildCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
