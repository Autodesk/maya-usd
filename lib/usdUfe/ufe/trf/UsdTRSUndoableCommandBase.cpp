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

#include "Utils.h"

#include <usdUfe/ufe/Utils.h>

#include <ufe/scene.h>
#include <ufe/sceneNotification.h>

namespace USDUFE_NS_DEF {

template <class V>
UsdTRSUndoableCommandBase<V>::UsdTRSUndoableCommandBase(double x, double y, double z)
    : _newValue(x, y, z)
{
}

template <class V> void UsdTRSUndoableCommandBase<V>::updateItem() const
{
    if (!_item) {
        auto ufeSceneItemPtr = Ufe::Hierarchy::createItem(getPath());
        _item = downcast(ufeSceneItemPtr);
    }
}

template <class V> void UsdTRSUndoableCommandBase<V>::initialize()
{
    if (cannotInit()) {
        return;
    }

    // If prim does not have the attribute, add it.
    if (!prim().HasAttribute(attributeName())) {
        _opAdded = true;
        addEmptyAttribute();
    }

    attribute().Get(&_prevValue);
}

template <class V> void UsdTRSUndoableCommandBase<V>::undoImp()
{
    // Set fItem to nullptr because the command does not know what can go on with the prim inside
    // its item after their own undo() or redo(). Setting it back to nullptr is safer because it
    // means that the next time the command is used, it will be forced to create a new item from the
    // path, or the command will crash on a null pointer.
    _item = nullptr;

    updateItem();

    attribute().Set(_prevValue);
    // Todo : We would want to remove the xformOp
    // (SD-06/07/2018) Haven't found a clean way to do it - would need to investigate
}

template <class V> void UsdTRSUndoableCommandBase<V>::redoImp()
{
    // Set fItem to nullptr because the command does not know what can go on with the prim inside
    // its item after their own undo() or redo(). Setting it back to nullptr is safer because it
    // means that the next time the command is used, it will be forced to create a new item from the
    // path, or the command will crash on a null pointer.
    _item = nullptr;

    updateItem();

    // We must go through conversion to the common transform API by calling
    // perform(), otherwise we get "Empty typeName" USD assertions for rotate
    // and scale.  Once that is done, we can simply set the attribute directly.
    if (_doneOnce) {
        attribute().Set(_newValue);
        return;
    }

    perform(_newValue[0], _newValue[1], _newValue[2]);
}

template <class V> void UsdTRSUndoableCommandBase<V>::perform(double x, double y, double z)
{
    _newValue = V(x, y, z);
    performImp(x, y, z);
    _doneOnce = true;
}

template <class V> bool UsdTRSUndoableCommandBase<V>::cannotInit() const { return false; }

template class UsdTRSUndoableCommandBase<PXR_NS::GfVec3f>;
template class UsdTRSUndoableCommandBase<PXR_NS::GfVec3d>;

} // namespace USDUFE_NS_DEF
