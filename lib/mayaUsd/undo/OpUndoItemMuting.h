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

#ifndef MAYAUSD_UNDO_OPUNDOINFOMUTING_H
#define MAYAUSD_UNDO_OPUNDOINFOMUTING_H

#include <mayaUsd/undo/OpUndoItemList.h>

namespace MAYAUSD_NS_DEF {

//! \brief Turn off undo info recording for a given scope.
//
// Useful if code implement their own undo/redo without using the undo info
// but calls functions that generate undo info items that need to be ignored.
//
// Since all OpUndoItem are added to the UsdUndoManager singleton, for code
// that don't want to be undoable, we need a way to clear the genarated undo
// items. That is what the OpUndoItemMuting class does: it will clear all undo
// items generated while a muting instance is created.
class MAYAUSD_CORE_PUBLIC OpUndoItemMuting
{
public:
    //! \brief Constructor extracts all undo info items.
    OpUndoItemMuting()
        : _preservedUndoInfo(std::move(OpUndoItemList::instance()))
    {
        OpUndoItemList::instance().clear();
    }

    //! \brief Destructor restores all preserved undo info items that were extracted.
    ~OpUndoItemMuting() { OpUndoItemList::instance() = std::move(_preservedUndoInfo); }

private:
    OpUndoItemList _preservedUndoInfo;
};

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_UNDO_OPUNDOINFOMUTING_H
