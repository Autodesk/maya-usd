//
// Copyright 2022 Autodesk
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

#include <mayaUsd/base/api.h>
#include <mayaUsd/undo/OpUndoItemList.h>

#include <ufe/path.h>
#include <ufe/undoableCommand.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Command to edit as Maya data and USD prim.
class MAYAUSD_CORE_PUBLIC EditAsMayaUfeCommand : public Ufe::UndoableCommand
{
public:
    using Ptr = std::shared_ptr<EditAsMayaUfeCommand>;

    //! Construct and destruct a EditAsMayaCommand. Does not execute it.
    EditAsMayaUfeCommand(const Ufe::Path& path);
    ~EditAsMayaUfeCommand() override;

    //! Create a EditAsMayaCommand. Does not execute it.
    static EditAsMayaUfeCommand::Ptr create(const Ufe::Path& path);

    void execute() override;
    void undo() override;
    void redo() override;

private:
    // Delete the copy/move constructors assignment operators.
    EditAsMayaUfeCommand(const EditAsMayaUfeCommand&) = delete;
    EditAsMayaUfeCommand& operator=(const EditAsMayaUfeCommand&) = delete;
    EditAsMayaUfeCommand(EditAsMayaUfeCommand&&) = delete;
    EditAsMayaUfeCommand& operator=(EditAsMayaUfeCommand&&) = delete;

    OpUndoItemList _undoItemList;
    Ufe::Path      _path;
};

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
