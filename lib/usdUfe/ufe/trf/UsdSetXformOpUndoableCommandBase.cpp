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

#include <usdUfe/base/tokens.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/undo/UsdUndoBlock.h>
#include <usdUfe/utils/editRouterContext.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace USDUFE_NS_DEF {

USDUFE_VERIFY_CLASS_SETUP(Ufe::SetVector3dUndoableCommand, UsdSetXformOpUndoableCommandBase);

UsdSetXformOpUndoableCommandBase::UsdSetXformOpUndoableCommandBase(
    const PXR_NS::VtValue&     value,
    const Ufe::Path&           path,
    const PXR_NS::UsdTimeCode& writeTime)
    : Ufe::SetVector3dUndoableCommand(path)
    , _writeTime(writeTime)
    , _newOpValue(value)
    , _isPrepared(false)
    , _canUpdateValue(true)
    , _opCreated(false)
{
}

UsdSetXformOpUndoableCommandBase::UsdSetXformOpUndoableCommandBase(
    const Ufe::Path&   path,
    const UsdTimeCode& writeTime)
    : UsdSetXformOpUndoableCommandBase({}, path, writeTime)
{
}

void UsdSetXformOpUndoableCommandBase::execute()
{
    OperationEditRouterContext editContext(EditRoutingTokens->RouteTransform, getPrim());

    // Create the attribute and cache the initial value,
    // if this is the first time we're executed, or redo
    // the attribute creation.
    recreateOpIfNeeded();

    // Set the new value.
    prepareAndSet(_newOpValue);
    _canUpdateValue = true;
}

void UsdSetXformOpUndoableCommandBase::undo()
{
    // If the command was never called at all, do nothing.
    // A DCC (such as Maya) can start by calling undo.
    if (!_isPrepared)
        return;

    OperationEditRouterContext editContext(EditRoutingTokens->RouteTransform, getPrim());

    // Restore the initial value and potentially remove the creatd attributes.
    setValue(_initialOpValue, _writeTime);
    removeOpIfNeeded();
    _canUpdateValue = false;
}

void UsdSetXformOpUndoableCommandBase::redo()
{
    OperationEditRouterContext editContext(EditRoutingTokens->RouteTransform, getPrim());

    // Redo the attribute creation if the attribute was already created
    // but then undone.
    recreateOpIfNeeded();

    // Set the new value, potentially creating the attribute if it
    // did not exists or caching the initial value if this is the
    // first time the command is executed, redone or undone.
    prepareAndSet(_newOpValue);
    _canUpdateValue = true;
}

UsdPrim UsdSetXformOpUndoableCommandBase::getPrim() const
{
    // Convert the UFE path to the corresponding USD prim.
    return ufePathToPrim(path());
}

void UsdSetXformOpUndoableCommandBase::updateNewValue(const VtValue& v)
{
    // Redo the attribute creation if the attribute was already created
    // but then undone.
    recreateOpIfNeeded();

    // Update the value that will be set.
    if (_canUpdateValue)
        _newOpValue = v;

    // Set the new value, potentially creating the attribute if it
    // did not exists or caching the initial value if this is the
    // first time the command is executed, redone or undone.
    prepareAndSet(_newOpValue);
    _canUpdateValue = true;
}

void UsdSetXformOpUndoableCommandBase::prepareAndSet(const VtValue& v)
{
    if (v.IsEmpty())
        return;

    prepareOpIfNeeded();
    setValue(v, _writeTime);
}

void UsdSetXformOpUndoableCommandBase::prepareOpIfNeeded()
{
    if (_isPrepared)
        return;

    createOpIfNeeded(_opCreationUndo);
    _initialOpValue = getValue(_writeTime);
    _isPrepared = true;
    _opCreated = true;
}

void UsdSetXformOpUndoableCommandBase::recreateOpIfNeeded()
{
    if (!_isPrepared)
        return;

    if (_opCreated)
        return;

    _opCreationUndo.redo();
    _opCreated = true;
}

void UsdSetXformOpUndoableCommandBase::removeOpIfNeeded()
{
    if (!_isPrepared)
        return;

    if (!_opCreated)
        return;

    _opCreationUndo.undo();
    _opCreated = false;
}

} // namespace USDUFE_NS_DEF
