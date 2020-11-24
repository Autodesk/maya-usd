//
// Copyright 2020 Autodesk
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

#ifndef MAYAUSD_UNDO_UNDOABLE_BLOCK_H
#define MAYAUSD_UNDO_UNDOABLE_BLOCK_H

#include "ForwardDeclares.h"

#include <mayaUsd/base/api.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

//! \brief UsdUndoableItem 
/*!
    This class stores the list of inverse edit functions that are invoked 
    on undo() / redo() call. This is the object that must be placed in Maya's undo stack.
*/
class MAYAUSD_CORE_PUBLIC UsdUndoableItem
{
public:
    using InvertFunc = std::function<void()>;
    using InvertFuncs = std::vector<InvertFunc>;

    // default constructor/destructor
     UsdUndoableItem() = default;
    ~UsdUndoableItem() = default;

    // default copy constructor/assignment operator
    UsdUndoableItem(const UsdUndoableItem&) = default;
    UsdUndoableItem& operator=(const UsdUndoableItem&) = default;

    // move constructor/assignment operator
    UsdUndoableItem(UsdUndoableItem&&) = default;
    UsdUndoableItem& operator=(UsdUndoableItem&&) = default;

    void undo();
    void redo();

private:
    friend class UsdUndoManager;

    void doInvert();

    InvertFuncs _invertFuncs;
};

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_UNDO_UNDOABLE_BLOCK_H
