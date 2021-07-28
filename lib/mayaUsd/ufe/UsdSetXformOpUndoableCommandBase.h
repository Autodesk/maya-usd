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
#pragma once

#include <mayaUsd/undo/UsdUndoableItem.h>

#include <pxr/usd/usd/timeCode.h>

#include <ufe/transform3dUndoableCommands.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Base class for TRS commands.
//
// Helper class to factor out common code for translate, rotate, scale
// undoable commands.  It is templated on the type of the transform op.
//
// Developing commands to work with Maya TRS commands is made more difficult
// because Maya calls undo(), but never calls redo(): it simply calls set()
// with the new value again.  We must distinguish cases where set() must
// capture state, so that undo() can completely remove any added primSpecs or
// attrSpecs.  This class implements state tracking to allow this: state is
// saved on transition between kInitial and kExecute.
// UsdTransform3dMayaXformStack has a state machine based implementation that
// avoids conditionals, but UsdSetXformOpUndoableCommandBase is less invasive
// from a development standpoint.

template <typename T>
class UsdSetXformOpUndoableCommandBase : public Ufe::SetVector3dUndoableCommand
{
    const PXR_NS::UsdTimeCode _readTime;
    const PXR_NS::UsdTimeCode _writeTime;
    MayaUsd::UsdUndoableItem  _undoableItem;
    enum State
    {
        kInitial,
        kInitialUndoCalled,
        kExecute,
        kUndone,
        kRedone
    };
    State _state { kInitial };

public:
    UsdSetXformOpUndoableCommandBase(const Ufe::Path& path, const PXR_NS::UsdTimeCode& writeTime);

    // Ufe::UndoableCommand overrides.
    // No-op: Maya calls set() rather than execute().
    void execute() override;
    void undo() override;
    // No-op: Maya calls set() rather than redo().
    void redo() override;

    PXR_NS::UsdTimeCode readTime() const { return _readTime; }
    PXR_NS::UsdTimeCode writeTime() const { return _writeTime; }

    virtual void setValue(const T&) = 0;

    void handleSet(const T& v);
};

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
