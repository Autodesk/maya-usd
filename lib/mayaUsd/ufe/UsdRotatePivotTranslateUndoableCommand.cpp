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

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdRotatePivotTranslateUndoableCommand::UsdRotatePivotTranslateUndoableCommand(
    const Ufe::Path& path)
    : Ufe::TranslateUndoableCommand(path)
    , fPath(path)
    , fNoPivotOp(false)
{
    // create a sceneItem on first access
    sceneItem();

    // Prim does not have a pivot translate attribute
    const TfToken xpivot("xformOp:translate:pivot");
    if (!prim().HasAttribute(xpivot)) {
        fNoPivotOp = true;

        // Add an empty pivot translate.
        rotatePivotTranslateOp(prim(), fPath, 0, 0, 0);
    }

    fPivotAttrib = prim().GetAttribute(xpivot);
    fPivotAttrib.Get<GfVec3f>(&fPrevPivotValue);
}

UsdRotatePivotTranslateUndoableCommand::~UsdRotatePivotTranslateUndoableCommand() { }

UsdSceneItem::Ptr UsdRotatePivotTranslateUndoableCommand::sceneItem() const
{
    if (!fItem) {
        fItem = std::dynamic_pointer_cast<UsdSceneItem>(Ufe::Hierarchy::createItem(fPath));
    }
    return fItem;
}

/*static*/
UsdRotatePivotTranslateUndoableCommand::Ptr
UsdRotatePivotTranslateUndoableCommand::create(const Ufe::Path& path)
{
    return std::make_shared<UsdRotatePivotTranslateUndoableCommand>(path);
}

void UsdRotatePivotTranslateUndoableCommand::undo()
{
    fPivotAttrib.Set(fPrevPivotValue);
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
    rotatePivotTranslateOp(prim(), fPath, x, y, z);
    return true;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
