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
#ifndef USDUFE_USDSETXFORMOPUNDOABLECOMMANDBASE_H
#define USDUFE_USDSETXFORMOPUNDOABLECOMMANDBASE_H

#include <usdUfe/base/api.h>
#include <usdUfe/undo/UsdUndoableItem.h>

#include <pxr/base/vt/value.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/timeCode.h>

#include <ufe/transform3dUndoableCommands.h>

namespace USDUFE_NS_DEF {

//! \brief Base class for TRS commands.
//
// Helper class to factor out common code for translate, rotate, scale
// undoable commands.  It is templated on the type of the transform op.
//
// We must do a careful dance due to historic reasons and the way Maya handle
// interactive commands:
//
//     - These commands can be wrapped inside other commands which may
//       use their own UsdUndoBlock. In particular, we must not try to
//       undo an attribute creation if it was not yet created.
//
//     - Maya can call undo and set-value before first executing the
//       command. In particular, when using manipulation tools, Maya
//       will usually do loops of undo/set-value/redo, thus beginning
//       by undoing a command that was never executed.
//
//     - As a general rule, when undoing, we want to remove any attributes
//       that were created when first executed.
//
//     - When redoing some commands after an undo, Maya will update the
//       value to be set with an incorrect value when operating in object
//       space, which must be ignored.
//
// Those things are what the prepare-op/recreate-op/remove-op functions are
// aimed to support. Also, we must only capture the initial value the first
// time the value is modified, to support both the inital undo/set-value and
// avoid losing the initial value on repeat set-value.
class USDUFE_PUBLIC UsdSetXformOpUndoableCommandBase : public Ufe::SetVector3dUndoableCommand
{
public:
    UsdSetXformOpUndoableCommandBase(
        const PXR_NS::VtValue&     newValue,
        const Ufe::Path&           path,
        const PXR_NS::UsdTimeCode& writeTime);
    UsdSetXformOpUndoableCommandBase(const Ufe::Path& path, const PXR_NS::UsdTimeCode& writeTime);

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdSetXformOpUndoableCommandBase);

    // Ufe::UndoableCommand overrides.
    void execute() override;
    void undo() override;
    void redo() override;

    PXR_NS::UsdTimeCode writeTime() const { return _writeTime; }

    // Retrieve the USD prim affected by the command.
    virtual PXR_NS::UsdPrim getPrim() const;

protected:
    // Create the XformOp attributes if they do not exists.
    // The attribute creation must be capture in the UsdUndoableItem by using a
    // UsdUndoBlock, so that removeOpIfNeeded and recreateOpIfNeeded can undo
    // and redo the attribute creation if needed.
    virtual void createOpIfNeeded(UsdUndoableItem&) = 0;

    // Get the attribute at the given time.
    virtual PXR_NS::VtValue getValue(const PXR_NS::UsdTimeCode& time) const = 0;

    // Set the attribute at the given time. The value is guaranteed to either be
    // the initial value that was returned by the getValue function above or a
    // new value passed to the updateNewValue function below. So you are guaranteed
    // that the type contained in the VtValue is the type you want.
    virtual void setValue(const PXR_NS::VtValue&, const PXR_NS::UsdTimeCode& time) = 0;

    // Function called by sub-classed when they want to set a new value.
    void updateNewValue(const PXR_NS::VtValue& v);

private:
    void prepareAndSet(const PXR_NS::VtValue&);

    // Create the XformOp attributes if they do not exists and cache the initial value.
    void prepareOpIfNeeded();

    // Recreate the attribute after being removed if it was created.
    void recreateOpIfNeeded();

    // Remove the attribute if it was created.
    void removeOpIfNeeded();

    const PXR_NS::UsdTimeCode _writeTime;
    PXR_NS::VtValue           _initialOpValue;
    PXR_NS::VtValue           _newOpValue;
    UsdUndoableItem           _opCreationUndo;
    bool                      _isPrepared;
    bool                      _canUpdateValue;
    bool                      _opCreated;
};

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDSETXFORMOPUNDOABLECOMMANDBASE_H
