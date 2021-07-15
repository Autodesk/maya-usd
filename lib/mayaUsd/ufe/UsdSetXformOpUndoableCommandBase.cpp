//
// Copyright 2021 Autodesk
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
#include "UsdSetXformOpUndoableCommandBase.h"

#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/undo/UsdUndoBlock.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
void warnUnimplemented(const char* msg) { TF_WARN("Illegal call to unimplemented %s", msg); }
} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

template <typename T>
UsdSetXformOpUndoableCommandBase<T>::UsdSetXformOpUndoableCommandBase(
    const Ufe::Path&   path,
    const UsdTimeCode& writeTime)
    : Ufe::SetVector3dUndoableCommand(path)
    , _readTime(getTime(path)) // Always read from proxy shape time.
    , _writeTime(writeTime)
{
}

template <typename T> void UsdSetXformOpUndoableCommandBase<T>::execute()
{
    warnUnimplemented("UsdSetXformOpUndoableCommandBase::execute()");
}

template <typename T> void UsdSetXformOpUndoableCommandBase<T>::undo()
{
    if (_state == kInitial) {
        // Spurious call from Maya, ignore.
        _state = kInitialUndoCalled;
        return;
    }
    _undoableItem.undo();
    _state = kUndone;
}

template <typename T> void UsdSetXformOpUndoableCommandBase<T>::redo()
{
    warnUnimplemented("UsdSetXformOpUndoableCommandBase::redo()");
}

template <typename T> void UsdSetXformOpUndoableCommandBase<T>::handleSet(const T& v)
{
    if (_state == kInitialUndoCalled) {
        // Spurious call from Maya, ignore.  Otherwise, we set a value that
        // is identical to the previous, the UsdUndoBlock does not capture
        // any invertFunc's, and subsequent undo() calls undo nothing.
        _state = kInitial;
    } else if (_state == kInitial) {
        UsdUndoBlock undoBlock(&_undoableItem);
        setValue(v);
        _state = kExecute;
    } else if (_state == kExecute) {
        setValue(v);
    } else if (_state == kUndone) {
        _undoableItem.redo();
        _state = kRedone;
    }
}

// Explicit instantiation for transform ops that can be set from matrices and
// vectors.
template class UsdSetXformOpUndoableCommandBase<GfVec3f>;
template class UsdSetXformOpUndoableCommandBase<GfVec3d>;
template class UsdSetXformOpUndoableCommandBase<GfMatrix4d>;

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
