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

#include "UsdTRSUndoableCommandBase.h"

#include "private/Utils.h"

#include <mayaUsd/ufe/Utils.h>

#include <ufe/scene.h>
#include <ufe/sceneNotification.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

template <class V>
UsdTRSUndoableCommandBase<V>::UsdTRSUndoableCommandBase(double x, double y, double z)
    : fNewValue(x, y, z)
{
}

template <class V> void UsdTRSUndoableCommandBase<V>::updateItem() const
{
    if (!fItem) {
        auto ufeSceneItemPtr = Ufe::Hierarchy::createItem(getPath());
        fItem = downcast(ufeSceneItemPtr);
    }
}

template <class V> void UsdTRSUndoableCommandBase<V>::initialize()
{
    if (cannotInit()) {
        return;
    }

    // If prim does not have the attribute, add it.
    if (!prim().HasAttribute(attributeName())) {
        fOpAdded = true;
        addEmptyAttribute();
    }

    attribute().Get(&fPrevValue);
}

template <class V> void UsdTRSUndoableCommandBase<V>::undoImp()
{
    // Set fItem to nullptr because the command does not know what can go on with the prim inside
    // its item after their own undo() or redo(). Setting it back to nullptr is safer because it
    // means that the next time the command is used, it will be forced to create a new item from the
    // path, or the command will crash on a null pointer.
    fItem = nullptr;

    updateItem();

    attribute().Set(fPrevValue);
    // Todo : We would want to remove the xformOp
    // (SD-06/07/2018) Haven't found a clean way to do it - would need to investigate
}

template <class V> void UsdTRSUndoableCommandBase<V>::redoImp()
{
    // Set fItem to nullptr because the command does not know what can go on with the prim inside
    // its item after their own undo() or redo(). Setting it back to nullptr is safer because it
    // means that the next time the command is used, it will be forced to create a new item from the
    // path, or the command will crash on a null pointer.
    fItem = nullptr;

    updateItem();

    // We must go through conversion to the common transform API by calling
    // perform(), otherwise we get "Empty typeName" USD assertions for rotate
    // and scale.  Once that is done, we can simply set the attribute directly.
    if (fDoneOnce) {
        attribute().Set(fNewValue);
        return;
    }

    perform(fNewValue[0], fNewValue[1], fNewValue[2]);
}

template <class V> void UsdTRSUndoableCommandBase<V>::perform(double x, double y, double z)
{
    fNewValue = V(x, y, z);
    performImp(x, y, z);
    fDoneOnce = true;
}

template <class V> bool UsdTRSUndoableCommandBase<V>::cannotInit() const { return false; }

template class UsdTRSUndoableCommandBase<GfVec3f>;
template class UsdTRSUndoableCommandBase<GfVec3d>;

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
