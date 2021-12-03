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
#include "OpUndoInfo.h"

namespace MAYAUSD_NS_DEF {

//------------------------------------------------------------------------------
// OpUndoInfo
//------------------------------------------------------------------------------

OpUndoInfo::~OpUndoInfo() { clear(); }

bool OpUndoInfo::undo()
{
    bool overallSuccess = true;
    // Note: iterate in reverse order since operations might depends on each other.
    const auto end = _undoItems.rend();
    for (auto iter = _undoItems.rbegin(); iter != end; ++iter)
        overallSuccess &= (*iter)->undo();

    _isUndone = true;

    return overallSuccess;
}

bool OpUndoInfo::redo()
{
    bool overallSuccess = true;
    for (auto& item : _undoItems)
        overallSuccess &= item->redo();

    _isUndone = false;

    return overallSuccess;
}

void OpUndoInfo::addItem(OpUndoItem::Ptr&& item)
{
    // Note: OpUndoItem::Ptr are unique_ptr, so we need to take ownership of them.
    _undoItems.emplace_back(std::move(item));
}

void OpUndoInfo::addDeleted(const MObjectHandle obj)
{
    _deletedMayaObjects.insert(obj);
}

bool OpUndoInfo::isDeleted(const MObjectHandle obj) const
{
   return _deletedMayaObjects.count(obj) > 0;
}

void OpUndoInfo::clear()
{
    // Note: we need to destroy the undo items in reverse order
    //       since some items might depends on previous ones.
    if (_isUndone) {
        const auto end = _undoItems.rend();
        for (auto iter = _undoItems.rbegin(); iter != end; ++iter)
            iter->reset();
    } else {
        const auto end = _undoItems.end();
        for (auto iter = _undoItems.begin(); iter != end; ++iter)
            iter->reset();
    }

    _undoItems.clear();
    _deletedMayaObjects.clear();
    _isUndone = false;
}

OpUndoInfo OpUndoInfo::extract()
{
    OpUndoInfo extracted;
    for (auto&& item : _undoItems)
        extracted._undoItems.emplace_back(std::move(item));

    _undoItems.clear();
    _deletedMayaObjects.clear();
    extracted._isUndone = _isUndone;
    
    return extracted;
}

} // namespace MAYAUSD_NS_DEF