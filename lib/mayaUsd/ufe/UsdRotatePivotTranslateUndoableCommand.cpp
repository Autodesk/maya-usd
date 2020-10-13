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

MAYAUSD_NS_DEF {
namespace ufe {

#ifdef UFE_V2_FEATURES_AVAILABLE
UsdRotatePivotTranslateUndoableCommand::UsdRotatePivotTranslateUndoableCommand(const Ufe::Path& path)
    : Ufe::TranslateUndoableCommand(path)
    , fPath(path)
    , fNoPivotOp(false)
#else
UsdRotatePivotTranslateUndoableCommand::UsdRotatePivotTranslateUndoableCommand(const UsdPrim& prim, const Ufe::Path& path, const Ufe::SceneItem::Ptr& item)
    : Ufe::TranslateUndoableCommand(item)
    , fPrim(prim)
    , fPath(path)
    , fNoPivotOp(false)
#endif
{
    #ifdef UFE_V2_FEATURES_AVAILABLE
    // create a sceneItem on first access
    sceneItem();
    #endif

    // Prim does not have a pivot translate attribute
    const TfToken xpivot("xformOp:translate:pivot");
    #ifdef UFE_V2_FEATURES_AVAILABLE
    if (!prim().HasAttribute(xpivot))
    #else
    if (!fPrim.HasAttribute(xpivot))
    #endif
    {
        fNoPivotOp = true;
        // Add an empty pivot translate.
        #ifdef UFE_V2_FEATURES_AVAILABLE
        rotatePivotTranslateOp(prim(), fPath, 0, 0, 0);
        #else
        rotatePivotTranslateOp(fPrim, fPath, 0, 0, 0);
        #endif
    }

    #ifdef UFE_V2_FEATURES_AVAILABLE
    fPivotAttrib = prim().GetAttribute(xpivot);
    #else
    fPivotAttrib = fPrim.GetAttribute(xpivot);
    #endif

    fPivotAttrib.Get<GfVec3f>(&fPrevPivotValue);
}

UsdRotatePivotTranslateUndoableCommand::~UsdRotatePivotTranslateUndoableCommand()
{
}

#ifdef UFE_V2_FEATURES_AVAILABLE
UsdSceneItem::Ptr
UsdRotatePivotTranslateUndoableCommand::sceneItem() const {
    if (!fItem) {
        fItem = std::dynamic_pointer_cast<UsdSceneItem>(Ufe::Hierarchy::createItem(fPath));
    }
    return fItem;
}
#endif

/*static*/
#ifdef UFE_V2_FEATURES_AVAILABLE
UsdRotatePivotTranslateUndoableCommand::Ptr UsdRotatePivotTranslateUndoableCommand::create(const Ufe::Path& path)
{
    return std::make_shared<UsdRotatePivotTranslateUndoableCommand>(path);
}
#else
UsdRotatePivotTranslateUndoableCommand::Ptr UsdRotatePivotTranslateUndoableCommand::create(const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item)
{
    return std::make_shared<UsdRotatePivotTranslateUndoableCommand>(prim, ufePath, item);
}
#endif

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

#if UFE_PREVIEW_VERSION_NUM >= 2025
bool UsdRotatePivotTranslateUndoableCommand::set(double x, double y, double z)
#else
bool UsdRotatePivotTranslateUndoableCommand::translate(double x, double y, double z)
#endif
{
    #ifdef UFE_V2_FEATURES_AVAILABLE
    rotatePivotTranslateOp(prim(), fPath, x, y, z);
    #else
    rotatePivotTranslateOp(fPrim, fPath, x, y, z);
    #endif
    
    return true;
}

} // namespace ufe
} // namespace MayaUsd
