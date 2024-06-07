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
#include "UsdRotatePivotTranslateUndoableCommand.h"

#include "private/Utils.h"

#include <mayaUsd/ufe/Utils.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_SETUP(Ufe::TranslateUndoableCommand, UsdRotatePivotTranslateUndoableCommand);

UsdRotatePivotTranslateUndoableCommand::UsdRotatePivotTranslateUndoableCommand(
    const Ufe::Path& path)
    : Ufe::TranslateUndoableCommand(path)
    , _path(path)
    , _noPivotOp(false)
{
    // create a sceneItem on first access
    sceneItem();

    // Prim does not have a pivot translate attribute
    const TfToken xpivot("xformOp:translate:pivot");
    if (!prim().HasAttribute(xpivot)) {
        _noPivotOp = true;

        // Add an empty pivot translate.
        UsdUfe::rotatePivotTranslateOp(prim(), _path, 0, 0, 0);
    }

    _pivotAttrib = prim().GetAttribute(xpivot);
    _pivotAttrib.Get<GfVec3f>(&_prevPivotValue);
}

UsdUfe::UsdSceneItem::Ptr UsdRotatePivotTranslateUndoableCommand::sceneItem() const
{
    if (!_item) {
        _item = downcast(Ufe::Hierarchy::createItem(_path));
    }
    return _item;
}

/*static*/
UsdRotatePivotTranslateUndoableCommand::Ptr
UsdRotatePivotTranslateUndoableCommand::create(const Ufe::Path& path)
{
    return std::make_shared<UsdRotatePivotTranslateUndoableCommand>(path);
}

void UsdRotatePivotTranslateUndoableCommand::undo()
{
    _pivotAttrib.Set(_prevPivotValue);
    // Todo : We would want to remove the xformOp
    // (SD-06/07/2018) Haven't found a clean way to do it - would need to investigate
}

void UsdRotatePivotTranslateUndoableCommand::redo()
{
    // No-op, use move to translate the rotate pivot of the object.
    // The Maya move command directly invokes our translate() method in its
    // redoIt(), which is invoked both for the inital move and the redo.
}

//------------------------------------------------------------------------------
// Ufe::TranslateUndoableCommand overrides
//------------------------------------------------------------------------------

bool UsdRotatePivotTranslateUndoableCommand::set(double x, double y, double z)
{
    UsdUfe::rotatePivotTranslateOp(prim(), _path, x, y, z);
    return true;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
