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
#ifndef MAYAUSD_UNDO_OP_UNDO_INFO_H
#define MAYAUSD_UNDO_OP_UNDO_INFO_H

#include <mayaUsd/base/api.h>

#include <maya/MObjectHandle.h>

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace MAYAUSD_NS_DEF {

//------------------------------------------------------------------------------
// OpUndoItem
//------------------------------------------------------------------------------

/// \class OpUndoItem
/// \brief Record data needed to undo or redo a single undo sub-operation.
///
/// See OpUndoItems.h for concrete implementations.

class OpUndoItem
{
public:
    typedef std::unique_ptr<OpUndoItem> Ptr;

    /// \brief construct a single named sub-operation.
    MAYAUSD_CORE_PUBLIC
    OpUndoItem(std::string name)
        : _name(std::move(name))
    {
    }

    MAYAUSD_CORE_PUBLIC
    virtual ~OpUndoItem() = default;

    /// \brief execute a single sub-operation. By default calls redo.
    MAYAUSD_CORE_PUBLIC
    virtual bool execute() { return redo(); }

    /// \brief undo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    virtual bool undo() = 0;

    /// \brief redo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    virtual bool redo() = 0;

    /// \brief get the undo item name, used for debugging and logging.
    MAYAUSD_CORE_PUBLIC
    const std::string& getName() const { return _name; }

private:
    // Name the undo items to help debugging, tracing and logging.
    const std::string _name;
};

//------------------------------------------------------------------------------
// OpUndoItemList
//------------------------------------------------------------------------------

/// \class OpUndoItemList
/// \brief Record everything needed to undo or redo a complete operation or command.

class OpUndoItemList
{
public:
    /// \brief construct an undo info.
    MAYAUSD_CORE_PUBLIC
    OpUndoItemList() = default;

    MAYAUSD_CORE_PUBLIC
    OpUndoItemList(OpUndoItemList&& other);

    MAYAUSD_CORE_PUBLIC
    OpUndoItemList& operator=(OpUndoItemList&& other);

    /// \brief destroy the undo info.
    MAYAUSD_CORE_PUBLIC
    ~OpUndoItemList();

    /// \brief undo a complete operation.
    MAYAUSD_CORE_PUBLIC
    bool undo();

    /// \brief redo a complete operation.
    MAYAUSD_CORE_PUBLIC
    bool redo();

    /// \brief add a undo item. This takes ownership of the item.
    MAYAUSD_CORE_PUBLIC
    void addItem(OpUndoItem::Ptr&& item);

    /// \brief clear all undo/redo information contained here.
    MAYAUSD_CORE_PUBLIC
    void clear();

    /// \brief returns the global instance.
    ///
    /// The undo list can later be extracted into a command to implement
    /// its undo and redo. See extract() above.
    MAYAUSD_CORE_PUBLIC
    static OpUndoItemList& instance();

private:
    OpUndoItemList(const OpUndoItemList&) = delete;
    OpUndoItemList& operator=(const OpUndoItemList&) = delete;

    std::vector<OpUndoItem::Ptr> _undoItems;
    bool                         _isUndone = false;
};

} // namespace MAYAUSD_NS_DEF

#endif
