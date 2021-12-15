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

#ifndef MAYAUSD_UNDO_OPUNDOINFORECORDER_H
#define MAYAUSD_UNDO_OPUNDOINFORECORDER_H

#include <mayaUsd/undo/OpUndoItemList.h>

namespace MAYAUSD_NS_DEF {

//! \brief Record and extract undo item in the scope where it is declared.
//
// Useful if code implement their undo/redo using the OpUndoItemList and need
// to relieably extract the undo items from the UsdUndoManager.
//
// It will transfer to the target OpUndoInfo all items generated while it
// exists. Meant to be used on the stack.
class MAYAUSD_CORE_PUBLIC OpUndoItemRecorder
{
public:
    //! \brief Starts recording undo info in the given container.
    OpUndoItemRecorder(OpUndoItemList& undoInfo);

    //! \brief Ends recording undo info.
    ~OpUndoItemRecorder();

    //! \brief Starts recording undo info in the given container.
    //
    // Discard any previously recorded info.
    void startUndoRecording();

    //! \brief Ends recording undo info immediately.
    //
    // No further undo info will be extracted.
    void endUndoRecording();

private:
    OpUndoItemRecorder(const OpUndoItemRecorder&) = delete;
    OpUndoItemRecorder operator=(const OpUndoItemRecorder&) = delete;

    bool            _isRecording = false;
    OpUndoItemList& _undoInfo;
};

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_UNDO_OPUNDOINFORECORDER_H
