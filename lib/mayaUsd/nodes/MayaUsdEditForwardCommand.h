//
// Copyright 2026 Autodesk
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
#ifndef MAYAUSD_EDITFORWARDCOMMAND_H
#define MAYAUSD_EDITFORWARDCOMMAND_H

#include <mayaUsd/base/api.h>

#include <usdUfe/undo/UsdUndoableItem.h>

#include <ufe/undoableCommand.h>

namespace MAYAUSD_NS_DEF {

//! \brief MayaUsdEditForwardCommand wraps a forwarded USD edit callback in an
//!        undoable UFE command, allowing edit-forwarding operations to be undoable.
class MAYAUSD_CORE_PUBLIC MayaUsdEditForwardCommand : public Ufe::UndoableCommand
{
public:
    typedef std::shared_ptr<MayaUsdEditForwardCommand> Ptr;

    using Callbacks = std::vector<std::function<void()>>;

    // Public for std::make_shared() access, use create() instead.
    MayaUsdEditForwardCommand(Callbacks callbacks);
    ~MayaUsdEditForwardCommand() = default;

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(MayaUsdEditForwardCommand);

    //! Create a MayaUsdEditForwardCommand wrapping multiple callbacks.
    static Ptr create(Callbacks callbacks);

    //! Create a MayaUsdEditForwardCommand wrapping a single callback.
    static Ptr create(const std::function<void()>& callback);

    std::string commandString() const override { return "Forward USD Edits"; }

private:
    void execute() override;
    void undo() override { _undoableItem.undo(); }
    void redo() override { _undoableItem.redo(); }

    UsdUfe::UsdUndoableItem _undoableItem;
    Callbacks               _callbacks;
};

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_EDITFORWARDCOMMAND_H
