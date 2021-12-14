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

#include <mayaUsd/undo/OpUndoItemRecorder.h>

namespace MAYAUSD_NS_DEF {

OpUndoItemRecorder::OpUndoItemRecorder(OpUndoItemList& undoInfo)
    : _isRecording(false)
    , _undoInfo(undoInfo)
{
    startUndoRecording();
}

OpUndoItemRecorder::~OpUndoItemRecorder() { endUndoRecording(); }

void OpUndoItemRecorder::startUndoRecording()
{
    // Don't do anything if recording already started.
    if (_isRecording)
        return;
    _isRecording = true;

    // Clear any previously-generated undo items, both the undo info container
    // and in the global container.
    _undoInfo.clear();
    OpUndoItemList::instance().clear();
}

void OpUndoItemRecorder::endUndoRecording()
{
    // Don't do anything if recording already ended.
    if (!_isRecording)
        return;
    _isRecording = false;

    // Extract the undo items from the global container
    // into the container we were given.
    _undoInfo = std::move(OpUndoItemList::instance());
    OpUndoItemList::instance().clear();
}

} // namespace MAYAUSD_NS_DEF
