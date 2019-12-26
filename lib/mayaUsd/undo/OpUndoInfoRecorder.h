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

#include <mayaUsd/undo/OpUndoInfo.h>
#include <mayaUsd/undo/UsdUndoManager.h>

namespace MAYAUSD_NS_DEF {

//! \brief Record and extract undo item in the scope where it is declared.
//
// Useful if code implement their undo/redo using the OpUndoInfo and need
// to relieably extract the undo items from the UsdUndoManager.

class MAYAUSD_CORE_PUBLIC OpUndoInfoRecorder
{
public:
    //! \brief Starts recording undo info in the given container.
    OpUndoInfoRecorder(OpUndoInfo& undoInfo);

    //! \brief Ends recording undo info.
    ~OpUndoInfoRecorder();

    //! \brief Starts recording undo info in the given container.
    //
    // Discard any previously recorded info.
    void startUndoRecording();

    //! \brief Ends recording undo info immediately.
    //
    // No further undo info will be extracted.
    void endUndoRecording();

private:
    OpUndoInfoRecorder(const OpUndoInfoRecorder&) = delete;
    OpUndoInfoRecorder operator=(const OpUndoInfoRecorder&) = delete;

    bool        _isRecording = false;
    OpUndoInfo& _undoInfo;
};

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_UNDO_OPUNDOINFORECORDER_H
