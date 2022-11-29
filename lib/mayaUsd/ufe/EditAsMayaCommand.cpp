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
#include "EditAsMayaCommand.h"

#include <mayaUsd/fileio/primUpdaterManager.h>
#include <mayaUsd/undo/OpUndoItemRecorder.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

EditAsMayaUfeCommand::Ptr EditAsMayaUfeCommand::create(const Ufe::Path& path)
{
    return std::make_shared<ufe::EditAsMayaUfeCommand>(path);
}

EditAsMayaUfeCommand::EditAsMayaUfeCommand(const Ufe::Path& path)
    : _path(path)
{
}

EditAsMayaUfeCommand::~EditAsMayaUfeCommand() { }

void EditAsMayaUfeCommand::execute()
{
    bool status = false;

    // Scope the undo item recording so we can undo on failure.
    {
        OpUndoItemRecorder undoRecorder(_undoItemList);

        auto& manager = PXR_NS::PrimUpdaterManager::getInstance();
        status = manager.editAsMaya(_path);
    }

    // Undo potentially partially-made edit-as-Maya on failure.
    if (!status)
        _undoItemList.undo();
}

void EditAsMayaUfeCommand::undo() { _undoItemList.redo(); }

void EditAsMayaUfeCommand::redo() { _undoItemList.redo(); }

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
