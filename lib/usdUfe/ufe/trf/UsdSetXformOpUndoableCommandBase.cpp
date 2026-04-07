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

#include <usdUfe/base/debugCodes.h>
#include <usdUfe/base/tokens.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/ufe/trf/Utils.h>
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
    // TfDebug::Enable(USDUFE_UNDOCMD);
}

UsdSetXformOpUndoableCommandBase::UsdSetXformOpUndoableCommandBase(
    const Ufe::Path&   path,
    const UsdTimeCode& writeTime)
    : UsdSetXformOpUndoableCommandBase({}, path, writeTime)
{
}

void UsdSetXformOpUndoableCommandBase::execute()
{
    TF_DEBUG_MSG(USDUFE_UNDOCMD, "Executing command\n");

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

    TF_DEBUG_MSG(USDUFE_UNDOCMD, "Undoing command\n");

    OperationEditRouterContext editContext(EditRoutingTokens->RouteTransform, getPrim());

    {
        // Restore the initial value.
        //
        // Note: the command does not use a UsdUndoBlock when setting values, so we
        //        must manually tell the edit-forwarding do do its work, but only
        //        for the value setting, not for the op creation/removal, which use
        //        an UsdUndoBlock.
        NoUsdUndoBlockGuard guard(true);
        setValue(_initialOpValue, _writeTime);
    }

    _opCreationUndo.undo();
    _canUpdateValue = false;
}

void UsdSetXformOpUndoableCommandBase::redo()
{
    // Note: various Maya commands call this `redo` function instead of `execute`
    //       in their `doIt` function.That's because many commands implement their
    //       `doIt` by calling their `redoIt`. Then they end-up calling `redo` on
    //       the UFE commands. Redirect to `execute` when we detect suck shenanigans,
    //       because our `execute` capture undo info but our `redo` replays them.
    if (!UsdUfe::isRedoing()) {
        execute();
        return;
    }

    TF_DEBUG_MSG(USDUFE_UNDOCMD, "Redoing command\n");

    OperationEditRouterContext editContext(EditRoutingTokens->RouteTransform, getPrim());

    _opCreationUndo.redo();

    {
        // Set the new value.
        //
        // Note: the command does not use a UsdUndoBlock when setting values, so we
        //        must manually tell the edit-forwarding do do its work, but only
        //        for the value setting, not for the op creation/removal, which use
        //        an UsdUndoBlock.
        NoUsdUndoBlockGuard guard(true);
        setValue(_newOpValue, _writeTime);
    }

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
    {
        // Set the new value.
        //
        // Note: the command does not use a UsdUndoBlock when setting values, so we
        //        must manually tell the edit-forwarding do do its work, but only
        //        for the value setting, not for the op creation/removal, which use
        //        an UsdUndoBlock.
        NoUsdUndoBlockGuard guard(true);
        setValue(v, _writeTime);
    }
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
